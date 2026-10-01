/*
 * PS5 Vulkan Template - the console: klog, the splash, the pad, time, the exit.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Standard error into klog (each line prefixed with the title's name), and
 * the shell's splash dismissed. Call first. */
void platform_init(const char *title_name);

/* One line to klog, flushed. */
void say(const char *format, ...) __attribute__((format(printf, 1, 2)));

double now_seconds(void);

/* The pad, in the console's button numbering. */
enum {
   PAD_L3 = 0x000002,
   PAD_R3 = 0x000004,
   PAD_OPTIONS = 0x000008,
   PAD_UP = 0x000010,
   PAD_RIGHT = 0x000020,
   PAD_DOWN = 0x000040,
   PAD_LEFT = 0x000080,
   PAD_L1 = 0x000400,
   PAD_R1 = 0x000800,
   PAD_TRIANGLE = 0x001000,
   PAD_CIRCLE = 0x002000,
   PAD_CROSS = 0x004000,
   PAD_SQUARE = 0x008000,
   PAD_TOUCH_PAD = 0x100000,
};

struct pad {
   uint32_t held;    /* buttons down now */
   uint32_t pressed; /* buttons that went down since the last poll */
   float left_x, left_y, right_x, right_y; /* -1..1, a dead zone removed; y is down-positive */
   float l2, r2;                           /* 0..1 */
};

/* Open the first user's pad; false (and no input) when there is none. */
bool pad_open(void);
void pad_poll(struct pad *pad);

#ifdef __cplusplus
}
#endif
