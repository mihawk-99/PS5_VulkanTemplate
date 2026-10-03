/*
 * Homebrew UI: BlackBearReloaded's designs, themes and components, drawn with Vulkan
 *
 * The gallery of PS5_VKHomebrewUI (my fork of ps5-homebrew-ui, with a Vulkan
 * backend) on the base class: twenty-one complete UI designs, thirty themes and
 * the component library, with the kit's sounds on the console's audio output.
 * The kit draws the whole screen, through ps5ui::KitExample (ps5/ui/kit.hpp).
 *
 * The pad belongs to the designs: L1 and R1 switch them, the touch pad shows
 * what each one demonstrates, and every other button is the design's own,
 * OPTIONS too. Holding OPTIONS for a second goes back to the menu (or ends a
 * title that has no other program).
 *
 * A test run's variant names a design ("uikit.aurora"): the gallery starts on
 * it and plays the kit's tour of it, the scripted input the kit's own pictures
 * are made with, so the frame the test saves is the same on every run.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

#ifndef PS5_APP_ROOT
#define PS5_APP_ROOT "/app0"
#endif

class VulkanExample : public ps5ui::KitExample
{
public:
	std::unique_ptr<hui::app::Shell> shell;
	std::unique_ptr<hui::app::Tour> tour;
	ps5ui::HoldToLeave leaving;
	hui::gfx::DrawList leavingList;
	// The frame rate the designs may show, averaged over half a second
	double fpsSeconds{ 0.0 };
	int fpsFrames{ 0 };
	uint64_t frames{ 0 };

	VulkanExample() : KitExample()
	{
		title = "Homebrew UI";
		name = "uikit";
		// The kit draws the whole screen and owns the pad
		settings.overlay = false;
		ps5.ownOverlay = true;
		defaultClearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit();
		// The kit's settings live in the title's folder; a test run starts from
		// none, in a folder of its own, so it is the same every time
		const std::string data = benchmark.active ? PS5_APP_ROOT "/hui-test" : PS5_APP_ROOT "/hui";
		hui::save::ensure_directory(data);
		chmod(data.c_str(), 0777);
		if (benchmark.active) {
			remove((data + "/settings.bin").c_str());
		}
		shell = std::make_unique<hui::app::Shell>(kit.fonts, kit.catalog, data, kit.renderer.glass_texture());
		shell->set_version("PS5 Vulkan Template");
		if (!ps5.variant.empty()) {
			tour = std::make_unique<hui::app::Tour>(*shell, ps5.variant);
			if (tour->finished()) {
				vks::tools::exitFatal("The gallery has no design named \"" + ps5.variant + "\"", -1);
			}
		}
		prepared = true;
	}

	// OPTIONS is the designs' (a pause menu, a settings drawer): holding it for
	// a second leaves, back to the start screen or, in a title of one program, out
	void holdOptions(float dt)
	{
		leaving.label = ps5.optionsEnds ? "Back to the start screen" : "Close";
		if (!ps5.frameBudget && leaving.update(dt, (ps5_pad().held & PAD_OPTIONS) != 0)) {
			quit = true;
		}
	}

	// The numbers some designs show (a frame time graph, draw calls). A test
	// run shows the same stand-in numbers as the kit's PC tool, so its
	// pictures do not depend on the console's timing.
	void updateTelemetry()
	{
		hui::app::Telemetry &telemetry = shell->telemetry();
		if (benchmark.active) {
			const float jitter = (float)((frames * 7919) % 97) / 97.0f;
			telemetry.push(16.3f + jitter * 0.8f + (frames % 211 == 0 ? 5.5f : 0.0f));
			telemetry.fps = 59.9f;
			telemetry.average_ms = 16.7f;
			telemetry.voices = (int)(frames / 40 % 4);
		} else {
			const float frameMs = frameTimer * 1000.0f;
			telemetry.push(frameMs);
			fpsSeconds += frameTimer;
			fpsFrames++;
			if (fpsSeconds >= 0.5) {
				telemetry.fps = (float)(fpsFrames / fpsSeconds);
				telemetry.average_ms = (float)(fpsSeconds * 1000.0 / fpsFrames);
				fpsSeconds = 0.0;
				fpsFrames = 0;
			}
			telemetry.voices = kit.mixer.active_voices();
		}
		telemetry.draw_calls = kit.renderer.last_draw_calls();
		telemetry.instances = kit.renderer.last_instances();
		frames++;
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		prepareFrame();
		// The frame's real length (1/60 s in a test run), clamped so a hitch
		// cannot teleport anything
		const float dt = std::min(frameTimer, 0.05f);
		hui::InputFrame input = kit.input();
		if (tour) {
			input = tour->step(dt);
		}
		holdOptions(dt);
		updateTelemetry();
		shell->update(input, dt);
		if (tour && !tour->capture().empty()) {
			tour->capture_done(); // the tour's pictures are the PC tool's; the test saves its own
		}
		if (shell->take_settings_changed()) {
			kit.apply(shell->settings());
		}
		kit.play(shell->feedback(), shell->sound_set(), shell->settings().haptics);
		if (shell->settings().light_bar) {
			kit.light_bar(shell->accent());
		}
		kit.tick(dt);
		shell->compose(kit.renderer);
		leavingList.clear();
		leaving.draw(leavingList, kit.fonts, ps5ui::active_theme());
		kit.renderer.draw(leavingList);
		buildKitCommandBuffer();
		submitFrame();
	}

	~VulkanExample() override
	{
		// The shell's draw lists are the kit's to play until the device is idle
		if (device) {
			vkDeviceWaitIdle(device);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
