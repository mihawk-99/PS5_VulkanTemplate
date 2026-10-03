/*
 * PS5 Vulkan Template - the title: the menu, its programs, and test runs.
 *
 * A launch by hand shows the menu (launcher.cpp) and runs the sample chosen
 * until OPTIONS is pressed, then shows the menu again. A title with one
 * program (src/samples.cpp) runs it at once instead, until it ends itself.
 *
 * A test run is a launch that finds /app0/test-run.txt, which ps5/tools/run.sh
 * writes just before it and the launch deletes:
 *
 *   frames 300                   frames each sample draws
 *   screenshot                   save each sample's last frame in /app0/screenshots/<id>.ppm
 *   samples all                  every sample linked in ("menu": the menu's; or ids;
 *                                "launcher": the menu itself, drawn for the budget)
 *
 * Each sample then runs in turn on a device of its own, and klog gets one line
 * for each ("sample <id>: ok, ..." or "sample <id>: FAILED, ...") and a summary.
 * The same lines go to /app0/test-results.txt as they happen, for a run with
 * no klog capture (ps5/tools/run.sh falls back to it).
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "ps5_samples.h"

#include <sys/stat.h>

#include <chrono>
#include <cstring>
#include <cstdarg>
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
const char *const resultsPath = PS5_APP_ROOT "/test-results.txt";
FILE *results = nullptr;

/* A test run's line: to klog, and to the results file, flushed at once so a
 * run that crashes keeps what came before. */
__attribute__((format(printf, 1, 2))) void report(const char *format, ...)
{
	char line[512];
	va_list args;
	va_start(args, format);
	vsnprintf(line, sizeof(line), format, args);
	va_end(args);
	say("%s", line);
	if (results) {
		fprintf(results, "%s\n", line);
		fflush(results);
	}
}
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
Result runSample(const Ps5Sample &sample, uint32_t frameBudget, const std::string &screenshotPath,
	bool optionsEnds = true, const std::string &variant = "")
{
	Result result;
	VulkanExampleBase *example = nullptr;
	// A variant is reported, and its picture saved, under "id.variant"
	const std::string name = variant.empty() ? sample.id : std::string(sample.id) + "." + variant;
	report("sample %s: starts", name.c_str());
	const double start = now();
	try {
		example = sample.create();
		// A test run is deterministic: upstream's samples seed their random
		// generators with 0 when benchmark.active is set, and the render loop
		// steps time by 1/60 s a frame (base/vulkanexamplebase.cpp)
		example->benchmark.active = frameBudget > 0;
		example->ps5.frameBudget = frameBudget;
		example->ps5.screenshotPath = screenshotPath;
		example->ps5.optionsEnds = optionsEnds;
		example->ps5.variant = variant;
		if (!example->initVulkan()) {
			throw std::runtime_error("initVulkan failed");
		}
		example->prepare();
		const double loopStart = now();
		example->renderLoop();
		const double end = now();
		result.frames = example->ps5.framesDrawn;
		result.seconds = end - start;
		// The frame rate once the first frames (pipeline compiles, uploads) are
		// behind it, up to the last frame (whose screenshot takes its time)
		const auto &times = example->ps5;
		if (times.halfwayTime > 0.0 && times.lastFrameTime > times.halfwayTime) {
			result.steadyFps = (frameBudget - 1 - frameBudget / 2) / (times.lastFrameTime - times.halfwayTime);
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
		report("sample %s: ok, %u frames in %.2f s, %.1f fps after the first half", name.c_str(), result.frames,
			result.seconds, result.steadyFps);
	} else {
		report("sample %s: FAILED after %.2f s: %s", name.c_str(), result.seconds, result.error.c_str());
	}
	return result;
}

/* The variants samples.cpp lists for a sample; none for most */
std::vector<std::string> variantsOf(const Ps5Sample &sample)
{
	std::vector<std::string> names;
	for (size_t i = 0; i < ps5VariantCount; i++) {
		if (strcmp(ps5Variants[i].id, sample.id) == 0) {
			std::istringstream words(ps5Variants[i].names);
			for (std::string name; words >> name;) {
				names.push_back(name);
			}
		}
	}
	return names;
}

struct Chosen {
	const Ps5Sample *sample;
	std::string variant;
	std::string name() const { return variant.empty() ? sample->id : std::string(sample->id) + "." + variant; }
};

int runTests(const TestRun &run)
{
	ps5TestRun = true;
	std::vector<Chosen> chosen;
	const bool launcher = std::find(run.samples.begin(), run.samples.end(), "launcher") != run.samples.end();
	bool start = false, themes = false;
#if defined(PS5_UI)
	start = std::find(run.samples.begin(), run.samples.end(), "start") != run.samples.end();
	themes = std::find(run.samples.begin(), run.samples.end(), "themes") != run.samples.end();
#endif
	for (const std::string &word : run.samples) {
		// "id.variant" names one variant; "all" and "menu" take every variant
		const size_t dot = word.find('.');
		const std::string id = word.substr(0, dot);
		for (size_t i = 0; i < ps5SampleCount; i++) {
			const Ps5Sample &sample = ps5Samples[i];
			if (word == "all" || (word == "menu" && sample.inMenu)) {
				const std::vector<std::string> variants = variantsOf(sample);
				if (variants.empty()) {
					chosen.push_back({ &sample, "" });
				}
				for (const std::string &variant : variants) {
					chosen.push_back({ &sample, variant });
				}
			} else if (id == sample.id) {
				chosen.push_back({ &sample, dot == std::string::npos ? "" : word.substr(dot + 1) });
			}
		}
	}
	if (chosen.empty() && !launcher && !start && !themes) {
		say("test run: no sample matches");
		return 1;
	}
	if (run.screenshot) {
		mkdir(screenshotDir, 0777);
		chmod(screenshotDir, 0777);
		// No picture of an earlier run may pass for this one's
		for (const Chosen &entry : chosen) {
			remove((std::string(screenshotDir) + "/" + entry.name() + ".ppm").c_str());
		}
	}
	results = fopen(resultsPath, "w");
	if (results) {
		chmod(resultsPath, 0666);
	}
	const size_t screens = (launcher ? 1 : 0) + (start ? 1 : 0) + (themes ? 1 : 0);
	report("test run: %zu samples, %u frames each%s", chosen.size() + screens, run.frames,
		run.screenshot ? ", a screenshot of each" : "");
	int failed = 0;
	if (launcher) {
		// The menu as a launch by hand shows it, ended by the budget instead of the pad
		report("sample launcher: starts");
		const double start = now();
		const std::string screenshot = run.screenshot ? std::string(screenshotDir) + "/launcher.ppm" : "";
		if (run.screenshot) {
			remove(screenshot.c_str());
		}
		ps5_run_launcher(-1, "", run.frames, screenshot);
		report("sample launcher: ok, %u frames in %.2f s", run.frames, now() - start);
	}
#if defined(PS5_UI)
	if (start) {
		// The start screen as a launch by hand shows it, ended by the budget
		report("sample start: starts");
		const double began = now();
		const std::string screenshot = run.screenshot ? std::string(screenshotDir) + "/start.ppm" : "";
		if (run.screenshot) {
			remove(screenshot.c_str());
		}
		ps5_run_start(0, run.frames, screenshot);
		report("sample start: ok, %u frames in %.2f s", run.frames, now() - began);
	}
	if (themes) {
		report("sample themes: starts");
		const double began = now();
		const std::string screenshot = run.screenshot ? std::string(screenshotDir) + "/themes.ppm" : "";
		if (run.screenshot) {
			remove(screenshot.c_str());
		}
		ps5_run_theme_picker(run.frames, screenshot);
		report("sample themes: ok, %u frames in %.2f s", run.frames, now() - began);
	}
#endif
	for (const Chosen &entry : chosen) {
		const std::string screenshot = run.screenshot ? std::string(screenshotDir) + "/" + entry.name() + ".ppm" : "";
		if (!runSample(*entry.sample, run.frames, screenshot, true, entry.variant).ok) {
			failed++;
		}
	}
	report("samples: %zu ok, %d failed", chosen.size() + screens - failed, failed);
	return failed ? 1 : 0;
}

/* The menu of samples, until Quit (or Back, when the start screen opened it) */
void runSamplesMenu(const char *leave)
{
	int selected = -1;
	std::string message;
	for (;;) {
		const int chosen = ps5_run_launcher(selected, message, 0, "", leave);
		if (chosen < 0) {
			return;
		}
		selected = chosen;
		const Result result = runSample(ps5Samples[chosen], 0, "");
		message = result.ok ? "" : std::string(ps5Samples[chosen].title) + " ended: " + result.error;
	}
}

void runMenu()
{
#if defined(PS5_UI)
	// With the UI module the title opens on its start screen: Samples (the menu
	// above), Designs (the kit's gallery) or Themes (the title's theme); each
	// comes back to it
	const Ps5Sample *gallery = nullptr;
	for (size_t i = 0; i < ps5SampleCount; i++) {
		if (strcmp(ps5Samples[i].id, "uikit") == 0) {
			gallery = &ps5Samples[i];
		}
	}
	if (gallery) {
		int choice = 0;
		for (;;) {
			choice = ps5_run_start(choice);
			if (choice < 0) {
				return;
			}
			if (choice == 1) {
				runSample(*gallery, 0, "");
			} else if (choice == 2) {
				ps5_run_theme_picker();
			} else {
				runSamplesMenu("Back");
			}
		}
	}
#endif
	runSamplesMenu("Quit");
}

} // namespace

int main()
{
	platform_init(PS5_TITLE_NAME);
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
	} else if (ps5SampleCount == 1) {
		// A title with one program: no menu, and the title ends with the program
		status = runSample(ps5Samples[0], 0, "", false).ok ? 0 : 1;
	} else {
		runMenu();
	}
	if (results) {
		report("ends: status %d", status);
		fclose(results);
	} else {
		say("ends: status %d", status);
	}
	return status;
}
