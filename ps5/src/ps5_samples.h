/*
 * PS5 Vulkan Template - what the title's parts share.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
#pragma once

#include "platform.h"

#include <cstddef>
#include <string>

class VulkanExampleBase;

/* One sample linked into the title (samples.cpp). */
struct Ps5Sample {
	const char *id;          // its folder under examples/ and shaders/glsl/
	const char *title;       // the name the menu shows
	const char *description; // one line under it
	bool inMenu;             // proven on the console (ps5/README.md); test runs reach every sample
	VulkanExampleBase *(*create)();
};

extern const Ps5Sample ps5Samples[];
extern const size_t ps5SampleCount;

/* The pad, shared by every sample and the launcher (example_ps5.cpp). */
struct pad &ps5_pad();

/* The menu (launcher.cpp): returns the index in ps5Samples of the sample
 * chosen, or -1 for Quit. The last result's message is shown under the list.
 * A test run's frame budget ends it after that many frames instead (-1), and
 * its last frame is saved to screenshotPath when that is not empty. */
int ps5_run_launcher(int selected, const std::string &message, uint32_t frameBudget = 0,
	const std::string &screenshotPath = "");
