/*
 * PS5 Vulkan Samples - the console: klog, the splash, the pad, time, the exit.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 *
 * Every console call here is one PS5_vkQuake, PS5_RetroArch or PS5_Vulkan's
 * smoke test runs with; no SDK header declares them.
 */
#include "platform.h"

#include <ps5platform/klog.h>

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

int sceSystemServiceHideSplashScreen(void);
int sceSystemServiceLoadExec(const char *path, const char *const *argv);
int sceUserServiceInitialize(const void *params);
int sceUserServiceGetInitialUser(int32_t *user_id);
int scePadInit(void);
int scePadOpen(int32_t user_id, int32_t port_type, int32_t index, const void *params);
int scePadRead(int32_t handle, void *samples, int32_t capacity);
int sceKernelUsleep(uint32_t microseconds);

void
platform_init(const char *title_name)
{
   static char prefix[64];
   snprintf(prefix, sizeof(prefix), "[%s] ", title_name);
   const int captured = ps5_klog_capture_stderr(prefix);
   say("starts (standard error to klog: %s)", captured == 0 ? "yes" : "no");
   /* The shell's splash covers the title's frames until it is dismissed. */
   say("splash dismissed: %d", sceSystemServiceHideSplashScreen());
}

void
say(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   vfprintf(stderr, format, args);
   va_end(args);
   fputc('\n', stderr);
   fflush(stderr);
}

double
now_seconds(void)
{
   struct timespec t;
   clock_gettime(CLOCK_MONOTONIC, &t);
   return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

/* The title ends by asking the shell to close it. exit() and returning from
 * _start both kill it, and the console then reports a crash over a run that
 * worked. The CRT calls this after main returns; the shell closes the title
 * asynchronously, so it must wait and never return. */
void
catchReturnFromMain(int status)
{
   say("exit status %d: asking the shell to close the title", status);
   fflush(NULL);
   (void)sceSystemServiceLoadExec("exit", NULL);
   for (;;)
      sceKernelUsleep(100000);
}

/* ----------------------------------------------------------------- the pad */

/* One sample as the console writes it: 120 bytes, the layout PS5_vkQuake and
 * ProsperoLight read. */
struct pad_sample {
   uint32_t buttons;
   uint8_t left_x, left_y, right_x, right_y, l2, r2;
   uint8_t reserved[66];
   int32_t connected;
   uint64_t timestamp_us;
   uint8_t extension[16];
   uint8_t connected_count;
   uint8_t remaining[15];
};
_Static_assert(sizeof(struct pad_sample) == 120, "the console's pad samples are 120 bytes");
_Static_assert(offsetof(struct pad_sample, connected) == 0x4c, "connection state sits at 0x4c");
_Static_assert(offsetof(struct pad_sample, timestamp_us) == 0x50, "the timestamp sits at 0x50");

/* Set while the shell has the pad (the home screen, a system dialog): such a
 * sample is not input for the title. */
#define PAD_INTERCEPTED 0x80000000u

static int32_t pad_handle = -1;
static struct pad_sample pad_last;

bool
pad_open(void)
{
   (void)sceUserServiceInitialize(NULL);
   int32_t user = -1;
   if (sceUserServiceGetInitialUser(&user) < 0 || scePadInit() < 0) {
      say("pad: no user or no pad service");
      return false;
   }
   /* A title can start before the pad service has published the device. */
   for (int attempt = 0; attempt < 10 && pad_handle < 0; attempt++) {
      pad_handle = scePadOpen(user, 0, 0, NULL);
      if (pad_handle < 0)
         sceKernelUsleep(100000);
   }
   say(pad_handle >= 0 ? "pad: opened for user %d" : "pad: scePadOpen failed for user %d", (int)user);
   memset(&pad_last, 0, sizeof(pad_last));
   pad_last.left_x = pad_last.left_y = pad_last.right_x = pad_last.right_y = 128;
   return pad_handle >= 0;
}

static float
stick(uint8_t value)
{
   const float v = ((float)value - 128.0f) / 127.0f;
   const float dead = 0.12f;
   if (v > -dead && v < dead)
      return 0.0f;
   return v > 0 ? (v - dead) / (1.0f - dead) : (v + dead) / (1.0f - dead);
}

void
pad_poll(struct pad *pad)
{
   const uint32_t before = pad->held;
   if (pad_handle >= 0) {
      static struct pad_sample samples[16];
      const int count = scePadRead(pad_handle, samples, 16);
      for (int i = 0; i < count && i < 16; i++) {
         if (samples[i].connected && !(samples[i].buttons & PAD_INTERCEPTED))
            pad_last = samples[i];
      }
   }
   pad->held = pad_handle >= 0 ? pad_last.buttons : 0;
   pad->pressed = pad->held & ~before;
   pad->left_x = stick(pad_last.left_x);
   pad->left_y = stick(pad_last.left_y);
   pad->right_x = stick(pad_last.right_x);
   pad->right_y = stick(pad_last.right_y);
   pad->l2 = pad_last.l2 / 255.0f;
   pad->r2 = pad_last.r2 / 255.0f;
}
