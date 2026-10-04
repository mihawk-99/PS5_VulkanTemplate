/*
 * PS5 Vulkan Template - the Samples menu, drawn with the UI kit in the title's theme.
 *
 * With the UI module, ps5_run_launcher (launcher.cpp) hands over to this menu:
 * the samples proven on the console in a themed list, the focused one described
 * beside it, and Back (to the start screen) or Quit at the foot. Like the menu it
 * replaces, it is a program on the base class that ends once a choice is made.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

namespace {

using hui::gfx::Color;

class SamplesMenu : public ps5ui::KitExample
{
public:
	int chosen{ -2 }; // -2: still choosing, -1: the last row, else an index in ps5Samples
	int selected;
	std::string message;
	const char *leave;
	std::vector<int> entries; // ps5Samples indices, in menu order
	const hui::ui::Theme &theme = ps5ui::active_theme();
	hui::ui::ListView list;
	hui::gfx::DrawList scene;
	hui::ui::Feedback feedback;
	float clock{ 0.0f };

	SamplesMenu(int selected, const std::string &message, const char *leave)
		: selected(selected), message(message), leave(leave)
	{
		title = PS5_TITLE_NAME;
		name = "samplesmenu";
		settings.overlay = false;
		ps5.ownOverlay = true;
		for (size_t i = 0; i < ps5SampleCount; i++) {
			if (ps5Samples[i].inMenu) {
				entries.push_back((int)i);
			}
		}
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit({ .music = false, .covers = false });
		std::vector<hui::ui::ListItem> rows;
		int focus = 0;
		for (size_t row = 0; row < entries.size(); row++) {
			const Ps5Sample &sample = ps5Samples[entries[row]];
			hui::ui::ListItem item;
			item.title = sample.title;
			item.subtitle = sample.description;
			item.tag = entries[row];
			rows.push_back(item);
			if (entries[row] == selected) {
				focus = (int)row;
			}
		}
		hui::ui::ListItem last;
		last.title = leave;
		last.tag = -1;
		rows.push_back(last);
		list.style.theme = theme;
		list.style.panel = true;
		list.style.row_height = 84.0f;
		list.style.title_size = 26.0f;
		list.style.subtitle_size = 19.0f;
		list.set_items(rows);
		list.set_bounds({ 120.0f, 230.0f, 1060.0f, 760.0f });
		list.set_focus(focus);
		list.enter();
		prepared = true;
	}

	void update(float dt)
	{
		feedback.clear();
		hui::InputFrame input = kit.input();
		if (ps5.frameBudget) {
			input = kit.scripted("launcher", ps5.framesDrawn);
		}
		clock += dt;
		const hui::ui::Event event = list.handle(input, feedback);
		if (event == hui::ui::Event::activated) {
			chosen = list.items()[list.focus()].tag;
		} else if (event == hui::ui::Event::cancelled || input.is_pressed(hui::Action::back)) {
			chosen = -1;
		}
		list.update(dt);
		kit.play(feedback, theme.sounds);
		kit.tick(dt);
	}

	void compose()
	{
		scene.clear();
		hui::ui::Canvas canvas{ scene, kit.fonts, 0, clock };
		hui::ui::Painter paint(scene, kit.fonts, theme);
		const bool fromStart = strcmp(leave, "Back") == 0;
		paint.heading(fromStart ? "Samples" : PS5_TITLE_NAME, 120.0f, 150.0f, 64.0f, paint.page_text());
		char device[160];
		snprintf(device, sizeof(device), "%s%s, Vulkan %u.%u.%u, %ux%u", fromStart ? PS5_TITLE_NAME ": " : "",
			deviceProperties.deviceName, VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
			VK_API_VERSION_MINOR(deviceProperties.apiVersion), VK_API_VERSION_PATCH(deviceProperties.apiVersion),
			width, height);
		paint.body(device, 122.0f, 196.0f, 22.0f, paint.page_text_muted());
		list.draw(canvas);

		// The focused sample, described beside the list
		const hui::gfx::Rect info{ 1240.0f, 230.0f, 560.0f, 520.0f };
		paint.panel(info);
		const hui::ui::ListItem &item = list.items()[list.focus()];
		paint.heading(item.title, info.x + 36.0f, info.y + 78.0f, 40.0f, theme.text);
		if (!item.subtitle.empty()) {
			float baseline = info.y + 130.0f;
			for (const std::string &line : kit.fonts.regular.font->wrap(item.subtitle, 24.0f, info.w - 72.0f)) {
				paint.body(line, info.x + 36.0f, baseline, 24.0f, theme.text_muted);
				baseline += 34.0f;
			}
			hui::ui::Theme panelInk = theme;
			panelInk.page_text_muted = theme.text_muted; // the hints sit on the panel, not the page
			ps5ui::draw_hints(scene, kit.fonts, panelInk,
				{ { hui::ui::Button::cross, "Start" }, { hui::ui::Button::options, "Back here, from the sample" } }, info.x + 36.0f, info.y + info.h - 52.0f, -1, 30.0f);
		} else {
			paint.body(fromStart ? "Back to the start screen" : "Close the title", info.x + 36.0f, info.y + 130.0f, 24.0f, theme.text_muted);
		}
		if (!message.empty()) {
			float baseline = info.y + info.h + 50.0f;
			for (const std::string &line : kit.fonts.regular.font->wrap(message, 22.0f, info.w)) {
				paint.body(line, info.x, baseline, 22.0f, theme.danger);
				baseline += 30.0f;
			}
		}
		ps5ui::draw_hints(scene, kit.fonts, theme,
			{ { hui::ui::Button::dpad, "Move" }, { hui::ui::Button::cross, "Start" }, { hui::ui::Button::circle, "Back" } }, 120.0f, 1040.0f);

		hui::gfx::BackdropSpec backdrop = theme.backdrop;
		backdrop.time = clock;
		kit.renderer.begin();
		kit.renderer.backdrop(backdrop);
		kit.renderer.draw(scene);
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		prepareFrame();
		update(std::min(frameTimer, 0.05f));
		compose();
		buildKitCommandBuffer();
		submitFrame();
		if (chosen != -2) {
			quit = true;
		}
	}

	~SamplesMenu() override
	{
		if (device) {
			vkDeviceWaitIdle(device);
		}
	}
};

} // namespace

int ps5_run_kit_launcher(int selected, const std::string &message, uint32_t frameBudget,
	const std::string &screenshotPath, const char *leave)
{
	SamplesMenu *menu = new SamplesMenu(selected, message, leave);
	menu->benchmark.active = frameBudget > 0;
	menu->ps5.frameBudget = frameBudget;
	menu->ps5.screenshotPath = screenshotPath;
	int chosen = -1;
	try {
		menu->initVulkan();
		menu->prepare();
		menu->renderLoop();
		chosen = frameBudget ? -1 : menu->chosen;
	} catch (const std::exception &e) {
		say("samples menu: %s", e.what());
	}
	if (menu->vulkanDevice) {
		vkDeviceWaitIdle(menu->vulkanDevice->logicalDevice);
	}
	delete menu;
	return chosen;
}
