/*
 * PS5 Vulkan Template - platform.h on a Linux PC, for the host reference build.
 *
 * The host reference build (ps5/tools/host-reference.sh) runs the title's code
 * on the PC's own Vulkan driver with a headless surface: no pad, messages on
 * standard error. Its pictures are the reference the console's are compared to.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
#include "platform.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *say_prefix = "";

void
platform_init(const char *title_name)
{
   static char prefix[64];
   snprintf(prefix, sizeof(prefix), "[%s] ", title_name);
   say_prefix = prefix;
   say("starts (host reference build)");
}

void
say(const char *format, ...)
{
   va_list args;
   va_start(args, format);
   fputs(say_prefix, stderr);
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

bool
pad_open(void)
{
   return false;
}

void
pad_poll(struct pad *pad)
{
   memset(pad, 0, sizeof(*pad));
}
