/*
 * PS5 Vulkan Template - Themes: the title's look, picked from the UI kit's thirty.
 *
 * The start screen's Themes. The themes in a list, the screen drawn in the one
 * the focus is on: its backdrop, its list, and a panel of the kit's widgets in
 * it. CROSS makes it the title's theme (ps5ui::set_active_theme), which styles
 * the start screen, the Samples menu and the samples' settings windows; CIRCLE
 * goes back to the start screen.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

namespace {

using hui::gfx::Rect;

class ThemePicker : public ps5ui::KitExample
{
public:
	bool done{ false };
	hui::ui::ListView list;
	hui::gfx::DrawList scene;
	hui::ui::Feedback feedback;
	float clock{ 0.0f };
	float applied{ 0.0f }; // seconds left of the "now in use" note

	ThemePicker()
	{
		title = PS5_TITLE_NAME;
		name = "themes";
		settings.overlay = false;
		ps5.ownOverlay = true;
	}

	const hui::ui::Theme &focused() const
	{
		return hui::ui::themes()[list.focus()];
	}

	void syncRows()
	{
		const int active = ps5ui::active_theme_index();
		for (int i = 0; i < (int)hui::ui::themes().size(); i++) {
			list.item(i).badge = i == active ? "IN USE" : "";
		}
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit({ .music = false, .covers = false });
		std::vector<hui::ui::ListItem> rows;
		for (const hui::ui::Theme &theme : hui::ui::themes()) {
			hui::ui::ListItem item;
			item.title = theme.name;
			item.subtitle = theme.family;
			rows.push_back(item);
		}
		list.style.panel = true;
		list.style.row_height = 76.0f;
		list.style.title_size = 26.0f;
		list.style.subtitle_size = 18.0f;
		list.set_items(rows);
		list.set_bounds({ 120.0f, 230.0f, 620.0f, 760.0f });
		list.set_focus(std::max(0, ps5ui::active_theme_index()));
		list.style.theme = focused();
		syncRows();
		list.enter();
		prepared = true;
	}

	void update(float dt)
	{
		feedback.clear();
		hui::InputFrame input = kit.input();
		if (ps5.frameBudget) {
			input = kit.scripted("themes", ps5.framesDrawn);
		}
		clock += dt;
		applied = std::max(0.0f, applied - dt);
		const hui::ui::Event event = list.handle(input, feedback);
		if (event == hui::ui::Event::activated) {
			ps5ui::set_active_theme(list.focus());
			syncRows();
			applied = 2.5f;
			feedback.play(hui::audio::Cue::saved);
		} else if (event == hui::ui::Event::cancelled || input.is_pressed(hui::Action::back)) {
			done = true;
		}
		// The whole screen wears the theme the focus is on
		list.style.theme = focused();
		list.update(dt);
		kit.play(feedback, focused().sounds);
		kit.tick(dt);
	}

	void compose()
	{
		const hui::ui::Theme &theme = focused();
		const uint32_t glass = kit.renderer.glass_texture();
		scene.clear();
		hui::ui::Canvas canvas{ scene, kit.fonts, glass, clock };
		hui::ui::Painter paint(scene, kit.fonts, theme, glass);
		paint.heading("Themes", 120.0f, 150.0f, 64.0f, paint.page_text());
		paint.body("The title's look: the start screen, the Samples menu and the samples' settings",
			122.0f, 196.0f, 22.0f, paint.page_text_muted());
		char count[32];
		snprintf(count, sizeof(count), "%02d / %02d", list.focus() + 1, (int)hui::ui::themes().size());
		paint.label(count, 1800.0f, 150.0f, 24.0f, paint.page_text_muted(), hui::gfx::Align::right);
		list.draw(canvas);

		// The kit's widgets in the theme, as a sample's settings would wear them
		const Rect panel{ 820.0f, 230.0f, 980.0f, 640.0f };
		paint.panel(panel);
		const float x = panel.x + 48.0f;
		paint.heading(theme.name, x, panel.y + 84.0f, 48.0f, theme.text);
		paint.body(theme.summary, x, panel.y + 128.0f, 22.0f, theme.text_muted);
		const char *tabs[] = { "Scene", "Lights", "Camera" };
		paint.tabs({ x, panel.y + 170.0f, 520.0f, 56.0f }, tabs, 0.0f, {});
		paint.label("Shadows", x, panel.y + 290.0f, 24.0f, theme.text);
		paint.toggle({ panel.x + panel.w - 48.0f - 92.0f, panel.y + 262.0f, 92.0f, 44.0f }, 1.0f, {});
		paint.label("Exposure", x, panel.y + 370.0f, 24.0f, theme.text);
		paint.slider({ x + 220.0f, panel.y + 346.0f, panel.w - 96.0f - 220.0f, 44.0f }, 0.62f, {});
		paint.checkbox({ x, panel.y + 420.0f, 40.0f, 40.0f }, 1.0f, {});
		paint.body("Bloom", x + 60.0f, panel.y + 450.0f, 24.0f, theme.text);
		paint.radio({ x + 260.0f, panel.y + 420.0f, 40.0f, 40.0f }, 1.0f, {});
		paint.body("MSAA 4x", x + 320.0f, panel.y + 450.0f, 24.0f, theme.text);
		paint.chip({ x + 520.0f, panel.y + 418.0f, 150.0f, 44.0f }, "Ray query", 1.0f, {});
		paint.button({ x, panel.y + panel.h - 112.0f, 260.0f, 64.0f }, "Apply", hui::ui::ButtonKind::primary, { 1.0f, 0.0f, false });
		paint.button({ x + 290.0f, panel.y + panel.h - 112.0f, 220.0f, 64.0f }, "Reset", hui::ui::ButtonKind::secondary, {});
		if (applied > 0.0f) {
			paint.label("Now the title's theme", panel.x + panel.w - 48.0f, panel.y + panel.h - 70.0f, 24.0f,
				theme.success, hui::gfx::Align::right);
		}
		ps5ui::draw_hints(scene, kit.fonts, theme,
			{ { hui::ui::Button::dpad, "Move" }, { hui::ui::Button::cross, "Use this theme" }, { hui::ui::Button::circle, "Back" } }, 120.0f, 1040.0f);

		hui::gfx::BackdropSpec backdrop = theme.backdrop;
		backdrop.time = clock;
		kit.renderer.begin();
		kit.renderer.backdrop(backdrop);
		kit.renderer.glass(); // glass themes frost their panels with the backdrop
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
		if (done) {
			quit = true;
		}
	}

	~ThemePicker() override
	{
		if (device) {
			vkDeviceWaitIdle(device);
		}
	}
};

} // namespace

void ps5_run_theme_picker(uint32_t frameBudget, const std::string &screenshotPath)
{
	ThemePicker *picker = new ThemePicker();
	picker->benchmark.active = frameBudget > 0;
	picker->ps5.frameBudget = frameBudget;
	picker->ps5.screenshotPath = screenshotPath;
	try {
		picker->initVulkan();
		picker->prepare();
		picker->renderLoop();
	} catch (const std::exception &e) {
		say("themes: %s", e.what());
	}
	if (picker->vulkanDevice) {
		vkDeviceWaitIdle(picker->vulkanDevice->logicalDevice);
	}
	delete picker;
}
