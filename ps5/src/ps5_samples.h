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

/* A sample's variants for test runs (samples.cpp): its id, and the variant
 * names separated by spaces. */
struct Ps5Variants {
	const char *id;
	const char *names;
};

extern const Ps5Variants ps5Variants[];
extern const size_t ps5VariantCount;

/* Set for the whole of a test run (main.cpp): what a person chose earlier (the
 * title's theme) must not change a test's pictures. */
extern bool ps5TestRun;

/* The pad, shared by every sample and the launcher (example_ps5.cpp). */
struct pad &ps5_pad();

/* The menu (launcher.cpp): returns the index in ps5Samples of the sample
 * chosen, or -1 for Quit. The last result's message is shown under the list.
 * A test run's frame budget ends it after that many frames instead (-1), and
 * its last frame is saved to screenshotPath when that is not empty. */
int ps5_run_launcher(int selected, const std::string &message, uint32_t frameBudget = 0,
	const std::string &screenshotPath = "", const char *leave = "Quit");

/* With the UI module (ps5/ui/): the start screen, which returns 0 for Samples, 1
 * for Designs, 2 for Themes and -1 for Quit; the Samples menu drawn with the kit,
 * which ps5_run_launcher hands over to; and the Themes picker. A test run's frame
 * budget ends each after that many frames, its last frame saved to screenshotPath. */
int ps5_run_start(int selected, uint32_t frameBudget = 0, const std::string &screenshotPath = "");
int ps5_run_kit_launcher(int selected, const std::string &message, uint32_t frameBudget,
	const std::string &screenshotPath, const char *leave);
void ps5_run_theme_picker(uint32_t frameBudget = 0, const std::string &screenshotPath = "");
