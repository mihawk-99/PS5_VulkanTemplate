/*
 * PS5 Vulkan Template - the start screen: Samples, Designs or Themes, drawn with the UI kit.
 *
 * The samples title opens on it when it has the UI module. Samples leads to the
 * menu of Vulkan samples, Designs to the kit's gallery of designs (uikit), Themes
 * to the picker of the title's theme, and each comes back here. Like the menu,
 * it is a program on the base class that ends once a choice is made, so what
 * runs next starts on a device of its own. It wears the title's theme; its
 * cards' artwork is drawn by the kit at start-up (render_to_texture).
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

#include <cmath>

namespace {

using hui::gfx::Color;
using hui::gfx::Rect;

enum Item { samplesItem, designsItem, themesItem, quitItem, itemCount };
constexpr int cardCount = 3;

class StartScreen : public ps5ui::KitExample
{
public:
	int chosen{ -2 }; // -2: still choosing, -1: Quit, else samplesItem, designsItem or themesItem
	int focus;
	int lastCard; // where the focus goes back to from Quit
	hui::tween::Spring shown[itemCount];
	uint32_t art[cardCount]{};
	const hui::ui::Theme &theme = ps5ui::active_theme();
	hui::gfx::DrawList scene;
	hui::ui::Feedback feedback;
	float clock{ 0.0f };

	explicit StartScreen(int selected)
		: focus(selected >= samplesItem && selected < cardCount ? selected : samplesItem), lastCard(focus)
	{
		title = PS5_TITLE_NAME;
		name = "start";
		settings.overlay = false;
		ps5.ownOverlay = true;
		defaultClearColor = { { 0.0f, 0.0f, 0.0f, 1.0f } };
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit({ .music = false, .covers = false });
		makeArt();
		for (int i = 0; i < itemCount; i++) {
			shown[i].snap(i == focus ? 1.0f : 0.0f);
		}
		prepared = true;
	}

	uint32_t paint(const hui::gfx::BackdropSpec &backdrop, const hui::gfx::DrawList &list)
	{
		kit.renderer.begin();
		kit.renderer.backdrop(backdrop);
		kit.renderer.draw(list);
		return kit.renderer.render_to_texture(800, 500, 800.0f, 500.0f);
	}

	// The cards' artwork: a procedural backdrop and a few shapes each
	void makeArt()
	{
		hui::gfx::DrawList list;

		// Samples: a warm synthwave floor under its sun
		hui::gfx::BackdropSpec floor;
		floor.mode = hui::gfx::BackdropMode::grid;
		floor.colors[0] = Color::rgb(0x0e0707);
		floor.colors[1] = Color::rgb(0x3a1610);
		floor.colors[2] = Color::rgb(0xff7a45);
		floor.colors[3] = Color::rgb(0xffc857);
		floor.time = 3.0f;
		art[samplesItem] = paint(floor, list);

		// Designs: one card in a fan of design languages, on light paper
		hui::gfx::BackdropSpec sheet;
		sheet.mode = hui::gfx::BackdropMode::paper;
		sheet.colors[0] = Color::rgb(0xf4efe4);
		sheet.colors[1] = Color::rgb(0xe3dccb);
		sheet.colors[2] = Color::rgb(0xffffff);
		const uint32_t fills[] = { 0xffb3c7, 0xffd166, 0x7cf0c8, 0x8fb8ff, 0xffffff };
		for (int i = 0; i < 5; i++) {
			const float angle = -0.42f + 0.21f * (float)i;
			const Rect card{ 250.0f + 40.0f * (float)i - 40.0f, 120.0f, 220.0f, 260.0f };
			list.shadow(card, 26.0f, 30.0f, Color::rgb(0x000000, 0.35f));
			list.rotated_rect(card, i == 4 ? 26.0f : 8.0f + 4.0f * (float)i, angle, Color::rgb(fills[i], i == 4 ? 1.0f : 0.9f));
		}
		list.rounded_rect({ 420.0f, 170.0f, 120.0f, 18.0f }, 9.0f, Color::rgb(0x2b2b33));
		list.rounded_rect({ 420.0f, 204.0f, 150.0f, 12.0f }, 6.0f, Color::rgb(0x2b2b33, 0.45f));
		list.arc(470.0f, 300.0f, 42.0f, 10.0f, 0.0f, 4.4f, Color::rgb(0xff5d8f));
		art[designsItem] = paint(sheet, list);

		// Themes: a swatch of every theme, its surface, its outline and its main colour
		hui::gfx::BackdropSpec dusk;
		dusk.mode = hui::gfx::BackdropMode::gradient;
		dusk.colors[0] = Color::rgb(0x1a1b22);
		dusk.colors[1] = Color::rgb(0x0b0b0e);
		list.clear();
		const auto all = hui::ui::themes();
		for (size_t i = 0; i < all.size(); i++) {
			const hui::ui::Theme &t = all[i];
			const float x = 70.0f + 112.0f * (float)(i % 6);
			const float y = 40.0f + 86.0f * (float)(i / 6);
			const Rect swatch{ x, y, 100.0f, 74.0f };
			list.rounded_rect(swatch.inset(-3.0f), 15.0f, t.page);
			list.bordered_rect(swatch, 12.0f, t.surface, 2.0f, t.outline.a > 0.0f ? t.outline : t.surface_high);
			list.rounded_rect({ x + 14.0f, y + 42.0f, 52.0f, 16.0f }, 8.0f, t.primary);
			list.rounded_rect({ x + 14.0f, y + 18.0f, 70.0f, 8.0f }, 4.0f, t.text.with_alpha(0.8f));
		}
		art[themesItem] = paint(dusk, list);
	}

	void move(int to)
	{
		if (to == focus) {
			return;
		}
		feedback.play(hui::audio::Cue::focus, 1.0f, to < cardCount ? 0.35f * (float)(to - 1) : 0.0f);
		focus = to;
		if (focus < cardCount) {
			lastCard = focus;
		}
	}

	void update(float dt)
	{
		feedback.clear();
		hui::InputFrame input = kit.input();
		if (ps5.frameBudget) {
			input = kit.scripted("start", ps5.framesDrawn);
		}
		clock += dt;
		const bool onCards = focus < cardCount;
		if (input.nav == hui::Direction::left && onCards && focus > 0) {
			move(focus - 1);
		} else if (input.nav == hui::Direction::right && onCards && focus < cardCount - 1) {
			move(focus + 1);
		} else if (input.nav == hui::Direction::down && onCards) {
			move(quitItem);
		} else if (input.nav == hui::Direction::up && !onCards) {
			move(lastCard);
		} else if (input.nav != hui::Direction::none && !input.nav_repeat) {
			feedback.play(hui::audio::Cue::error, 1.0f, 0.0f, 0.5f);
		}
		if (input.is_pressed(hui::Action::confirm)) {
			feedback.play(hui::audio::Cue::select);
			chosen = focus == quitItem ? -1 : focus;
		}
		for (int i = 0; i < itemCount; i++) {
			shown[i].target = i == focus ? 1.0f : 0.0f;
			shown[i].update(dt, theme.omega);
		}
		kit.play(feedback, theme.sounds);
		kit.tick(dt);
	}

	void compose()
	{
		const uint32_t glass = kit.renderer.glass_texture();
		scene.clear();
		hui::ui::Canvas canvas{ scene, kit.fonts, glass, clock };
		hui::ui::Painter painter(scene, kit.fonts, theme, glass);
		const hui::ui::ComponentStyle style{ theme };

		// The entrance: the title fades in, then the cards rise one after another
		const auto arrive = [this](float delay, float length) {
			return hui::tween::smoothstep(std::clamp((clock - delay) / length, 0.0f, 1.0f));
		};
		const float heading = arrive(0.0f, 0.5f);
		scene.push_opacity(heading);
		scene.push_transform(1.0f, 0.0f, 0.0f, 0.0f, (1.0f - heading) * 24.0f);
		painter.label("PLAYSTATION 5 HOMEBREW", 160.0f, 150.0f, 22.0f, painter.page_text_muted());
		painter.heading(PS5_TITLE_NAME, 156.0f, 252.0f, 96.0f, painter.page_text());
		painter.body("Vulkan 1.4 on RADV, at 3840 x 2160: samples of the API, interface designs and thirty themes",
			160.0f, 308.0f, 26.0f, painter.page_text_muted());
		scene.pop_transform();
		scene.pop_opacity();

		hui::ui::CardLook look;
		look.art_aspect = 1.6f;
		look.title_size = 40.0f;
		look.subtitle_size = 22.0f;
		look.text_gap = 18.0f;
		look.glow = true;
		const char *titles[cardCount] = { "Samples", "Designs", "Themes" };
		const char *subtitles[cardCount] = { "Vulkan techniques, from glTF to ray queries",
			"Twenty-one complete interface designs", "The title's look, from thirty" };
		const Color accents[cardCount] = { Color::rgb(0xff8a50), Color::rgb(0xffd166), theme.accent };
		const float cardWidth = 500.0f;
		const float gap = 50.0f;
		const float cardHeight = hui::ui::card_height(look, cardWidth);
		for (int i = 0; i < cardCount; i++) {
			hui::ui::CardItem item;
			item.title = titles[i];
			item.subtitle = subtitles[i];
			item.texture = art[i];
			item.uv = hui::gfx::kCanvasUv;
			item.image_aspect = 1.6f;
			item.accent = accents[i];
			hui::ui::CardState state;
			state.focus = shown[i].value;
			const Rect card{ 160.0f + (cardWidth + gap) * (float)i, 400.0f, cardWidth, cardHeight };
			const float rise = arrive(0.25f + 0.12f * (float)i, 0.55f);
			scene.push_opacity(rise);
			scene.push_transform(1.0f, 0.0f, 0.0f, 0.0f, (1.0f - rise) * 120.0f);
			hui::ui::draw_card(canvas, style, look, card, item, state);
			scene.pop_transform();
			scene.pop_opacity();
		}

		const float foot = arrive(0.75f, 0.4f);
		scene.push_opacity(foot);
		painter.button({ 860.0f, 940.0f, 200.0f, 60.0f }, "Quit", hui::ui::ButtonKind::secondary, { shown[quitItem].value, 0.0f, false });
		ps5ui::draw_hints(scene, kit.fonts, theme,
			{ { hui::ui::Button::dpad, "Move" }, { hui::ui::Button::cross, "Choose" }, { hui::ui::Button::circle, "Back here, from each" },
				{ hui::ui::Button::options, "Hold: back here, always" } }, 960.0f, 1040.0f, 0);
		scene.pop_opacity();

		hui::gfx::BackdropSpec backdrop = theme.backdrop;
		backdrop.time = clock;
		kit.renderer.begin();
		kit.renderer.backdrop(backdrop);
		kit.renderer.glass();
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

	~StartScreen() override
	{
		if (device) {
			vkDeviceWaitIdle(device);
		}
	}
};

} // namespace

int ps5_run_start(int selected, uint32_t frameBudget, const std::string &screenshotPath)
{
	StartScreen *start = new StartScreen(selected);
	start->benchmark.active = frameBudget > 0;
	start->ps5.frameBudget = frameBudget;
	start->ps5.screenshotPath = screenshotPath;
	int chosen = -1;
	try {
		start->initVulkan();
		start->prepare();
		start->renderLoop();
		chosen = frameBudget ? -1 : start->chosen;
	} catch (const std::exception &e) {
		say("start screen: %s", e.what());
	}
	if (start->vulkanDevice) {
		vkDeviceWaitIdle(start->vulkanDevice->logicalDevice);
	}
	delete start;
	return chosen;
}
