/*
 * PS5 Vulkan Samples - the title: the menu, the samples, and test runs.
 *
 * A launch by hand shows the menu (launcher.cpp) and runs the sample chosen
 * until OPTIONS is pressed, then shows the menu again.
 *
 * A test run is a launch that finds /app0/test-run.txt, which ps5/tools/run.sh
 * writes just before it and the launch deletes:
 *
 *   frames 300                   frames each sample draws
 *   screenshot                   save each sample's last frame in /app0/screenshots/<id>.ppm
 *   samples all                  every sample linked in ("menu": the menu's; or ids)
 *
 * Each sample then runs in turn on a device of its own, and klog gets one line
 * for each ("sample <id>: ok, ..." or "sample <id>: FAILED, ...") and a summary.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "ps5_samples.h"

#include <sys/stat.h>

#include <chrono>
#include <fstream>
#include <sstream>

#if !defined(PS5_HOST_REFERENCE)
extern "C" VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_icdGetInstanceProcAddr(VkInstance instance, const char *name);
#endif

/* The title's folder: /app0 on the console, a build folder in the host reference build */
#ifndef PS5_APP_ROOT
#define PS5_APP_ROOT "/app0"
#endif

namespace {

const char *const testRunPath = PS5_APP_ROOT "/test-run.txt";
const char *const screenshotDir = PS5_APP_ROOT "/screenshots";

struct TestRun {
	uint32_t frames{ 0 };
	bool screenshot{ false };
	std::vector<std::string> samples;
};

bool readTestRun(TestRun &run)
{
	std::ifstream file(testRunPath);
	if (!file) {
		return false;
	}
	std::string line;
	while (std::getline(file, line)) {
		std::istringstream words(line);
		std::string key;
		if (!(words >> key)) {
			continue;
		}
		if (key == "frames") {
			words >> run.frames;
		} else if (key == "screenshot") {
			run.screenshot = true;
		} else if (key == "samples") {
			for (std::string id; words >> id;) {
				run.samples.push_back(id);
			}
		} else {
			say("test run: unknown line \"%s\"", line.c_str());
		}
	}
	file.close();
	remove(testRunPath);
	if (run.frames == 0) {
		run.frames = 300;
	}
	return true;
}

struct Result {
	bool ok{ false };
	uint32_t frames{ 0 };
	double seconds{ 0.0 };
	double steadyFps{ 0.0 };
	std::string error;
};

double now()
{
	return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/* One sample from start to end, on an instance and a device of its own. A
 * fatal error in it (vks::tools::exitFatal) arrives here as an exception: the
 * sample is ended and the title carries on. */
Result runSample(const Ps5Sample &sample, uint32_t frameBudget, const std::string &screenshotPath)
{
	Result result;
	VulkanExampleBase *example = nullptr;
	say("sample %s: starts", sample.id);
	const double start = now();
	try {
		example = sample.create();
		// A test run is deterministic: upstream's samples seed their random
		// generators with 0 when benchmark.active is set, and the render loop
		// steps time by 1/60 s a frame (base/vulkanexamplebase.cpp)
		example->benchmark.active = frameBudget > 0;
		example->ps5.frameBudget = frameBudget;
		example->ps5.screenshotPath = screenshotPath;
		if (!example->initVulkan()) {
			throw std::runtime_error("initVulkan failed");
		}
		example->prepare();
		const double loopStart = now();
		example->renderLoop();
		const double end = now();
		result.frames = example->ps5.framesDrawn;
		result.seconds = end - start;
		// The frame rate once the first frames (pipeline compiles, uploads) are behind it
		if (example->ps5.halfwayTime > 0.0 && result.frames > 1) {
			result.steadyFps = (result.frames - result.frames / 2) / (end - example->ps5.halfwayTime);
		} else if (end > loopStart) {
			result.steadyFps = result.frames / (end - loopStart);
		}
		result.ok = true;
	} catch (const std::exception &e) {
		result.error = e.what();
		result.seconds = now() - start;
	}
	if (example) {
		if (example->vulkanDevice) {
			vkDeviceWaitIdle(example->vulkanDevice->logicalDevice);
		}
		delete example;
	}
	if (result.ok) {
		say("sample %s: ok, %u frames in %.2f s, %.1f fps after the first half", sample.id, result.frames,
			result.seconds, result.steadyFps);
	} else {
		say("sample %s: FAILED after %.2f s: %s", sample.id, result.seconds, result.error.c_str());
	}
	return result;
}

int runTests(const TestRun &run)
{
	std::vector<const Ps5Sample *> chosen;
	for (const std::string &id : run.samples) {
		for (size_t i = 0; i < ps5SampleCount; i++) {
			if (id == "all" || (id == "menu" && ps5Samples[i].inMenu) || id == ps5Samples[i].id) {
				chosen.push_back(&ps5Samples[i]);
			}
		}
	}
	if (chosen.empty()) {
		say("test run: no sample matches");
		return 1;
	}
	if (run.screenshot) {
		mkdir(screenshotDir, 0777);
		chmod(screenshotDir, 0777);
		// No picture of an earlier run may pass for this one's
		for (const Ps5Sample *sample : chosen) {
			remove((std::string(screenshotDir) + "/" + sample->id + ".ppm").c_str());
		}
	}
	say("test run: %zu samples, %u frames each%s", chosen.size(), run.frames,
		run.screenshot ? ", a screenshot of each" : "");
	int failed = 0;
	for (const Ps5Sample *sample : chosen) {
		const std::string screenshot = run.screenshot ? std::string(screenshotDir) + "/" + sample->id + ".ppm" : "";
		if (!runSample(*sample, run.frames, screenshot).ok) {
			failed++;
		}
	}
	say("samples: %zu ok, %d failed", chosen.size() - failed, failed);
	return failed ? 1 : 0;
}

void runMenu()
{
	int selected = -1;
	std::string message;
	for (;;) {
		const int chosen = ps5_run_launcher(selected, message);
		if (chosen < 0) {
			return;
		}
		selected = chosen;
		const Result result = runSample(ps5Samples[chosen], 0, "");
		message = result.ok ? "" : std::string(ps5Samples[chosen].title) + " ended: " + result.error;
	}
}

} // namespace

int main()
{
	platform_init("PS5 Vulkan Samples");
	// The samples report on std::cout and std::cerr: both reach klog
	std::cout.rdbuf(std::cerr.rdbuf());
	pad_open();
#if defined(PS5_HOST_REFERENCE)
	// The PC's Vulkan loader
	if (volkInitialize() != VK_SUCCESS) {
		say("no Vulkan loader");
		return 1;
	}
#else
	// Vulkan's commands come from the RADV linked into the title, through volk
	volkInitializeCustom(vk_icdGetInstanceProcAddr);
#endif

	int status = 0;
	TestRun run;
	if (readTestRun(run)) {
		status = runTests(run);
	} else {
		runMenu();
	}
	say("ends: status %d", status);
	return status;
}
