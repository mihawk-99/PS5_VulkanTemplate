/*
 * PS5 Vulkan Template - the UI module: the kit's system calls, on platform.h.
 *
 * The kit logs, reads the clock and sleeps through five functions of its own
 * (platform/ps5/system.hpp in PS5_VKHomebrewUI). Here they are the title's
 * klog and clock. The title already dismissed the splash, and it ends by
 * returning from its render loop, so park and quit are never the way out;
 * they keep the kit's contract all the same.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "platform.h"
#include "platform/ps5/system.hpp"

#include <cstdarg>
#include <cstdio>
#include <unistd.h>

#if !defined(PS5_HOST_REFERENCE)
extern "C" void catchReturnFromMain(int status);
#endif

namespace hui::sys
{

std::int64_t monotonic_us()
{
	return static_cast<std::int64_t>(now_seconds() * 1e6);
}

void log(const char *format, ...)
{
	char line[512];
	va_list args;
	va_start(args, format);
	std::vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	say("%s", line);
}

bool hide_splash_screen()
{
	return true; // platform_init has
}

void sleep_us(std::uint32_t microseconds)
{
	usleep(microseconds);
}

void park()
{
	for (;;) {
		usleep(100000);
	}
}

void quit()
{
#if !defined(PS5_HOST_REFERENCE)
	catchReturnFromMain(0); // asks the shell to close the title; never returns
#endif
	park();
}

} // namespace hui::sys
