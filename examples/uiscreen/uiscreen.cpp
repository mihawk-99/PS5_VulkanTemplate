/*
 * UI starter - the program a homebrew whose face is one of the kit's designs grows from.
 *
 * One complete design of PS5_VKHomebrewUI's kit (ps5/ui/kit.hpp) fills the
 * screen: its procedural backdrop, its scene, its frosted overlay and its post
 * overlay, its sounds on the console's audio output, its rumble and its light
 * bar colour. There is no switcher: every button is the design's, and holding
 * OPTIONS for a second leaves (back to the menu, or out of a title of one
 * program). Here the design is the kit's own Aurora Shelf, a console home
 * screen; ps5/tools/new-title.py --ui <design> makes a title whose program is
 * this file and whose design is a copy of the one named, in kit/screen.cpp,
 * to change at will.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

// The design on screen
std::unique_ptr<hui::app::Concept> makeScreen(hui::app::Context &context)
{
	return hui::concepts::make_aurora(context);
}

class VulkanExample : public ps5ui::KitExample
{
public:
	// What a design receives: the fonts, the sample content, the live numbers
	// it may show and the settings it may change
	hui::Settings choices;
	hui::app::Telemetry telemetry;
	std::unique_ptr<hui::app::Context> context;
	std::unique_ptr<hui::app::Concept> screen;
	hui::app::Frame frame;
	hui::ui::Feedback feedback;
	float optionsHeld{ 0.0f };
	double fpsSeconds{ 0.0 };
	int fpsFrames{ 0 };

	VulkanExample() : KitExample()
	{
		title = "UI starter";
		name = "uiscreen";
		// The kit draws the whole screen and owns the pad
		settings.overlay = false;
		ps5.ownOverlay = true;
		defaultClearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit();
		context = std::make_unique<hui::app::Context>(hui::app::Context{ kit.fonts, kit.catalog, telemetry, choices });
		screen = makeScreen(*context);
		screen->enter();
		kit.apply(choices);
		feedback.play(hui::audio::Cue::welcome);
		prepared = true;
	}

	// Holding OPTIONS for a second leaves; a press is the design's
	void holdOptions(float dt)
	{
		if (ps5.frameBudget) {
			return;
		}
		optionsHeld = (ps5_pad().held & PAD_OPTIONS) ? optionsHeld + dt : 0.0f;
		if (optionsHeld >= 1.0f) {
			quit = true;
		}
	}

	void updateTelemetry()
	{
		if (benchmark.active) {
			// A test run's pictures must not depend on the console's timing
			telemetry.push(16.7f);
			telemetry.fps = 59.9f;
			telemetry.average_ms = 16.7f;
		} else {
			telemetry.push(frameTimer * 1000.0f);
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
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		prepareFrame();
		const float dt = std::min(frameTimer, 0.05f);
		// A test run ignores the pad: the design shows its entrance and rests
		hui::InputFrame input = kit.input();
		if (ps5.frameBudget) {
			input = hui::InputFrame{};
			input.connected = true;
		}
		holdOptions(dt);
		updateTelemetry();
		screen->update(input, dt, feedback);
		if (context->settings_changed) {
			context->settings_changed = false;
			kit.apply(choices);
		}
		kit.play(feedback, screen->info().sounds, choices.haptics);
		feedback.clear();
		if (choices.light_bar) {
			kit.light_bar(screen->info().accent);
		}
		kit.tick(dt);

		// backdrop, scene, [the glass copy], overlay, post: the order a design is drawn in
		frame.reset();
		frame.glass_texture = kit.renderer.glass_texture();
		screen->draw(frame);
		kit.renderer.begin();
		kit.renderer.backdrop(frame.backdrop);
		kit.renderer.draw(frame.scene);
		if (frame.glass) {
			kit.renderer.glass();
		}
		kit.renderer.draw(frame.overlay);
		kit.renderer.backdrop(frame.post);
		buildKitCommandBuffer();
		submitFrame();
	}

	~VulkanExample() override
	{
		if (device) {
			vkDeviceWaitIdle(device);
		}
	}
};

VULKAN_EXAMPLE_MAIN()
