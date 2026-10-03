/*
 * PS5 Vulkan Template - the UI module: PS5_VKHomebrewUI's kit on the base class.
 *
 * A program that draws with the kit derives from ps5ui::KitExample instead of
 * VulkanExampleBase. KitExample owns a Kit: the Vulkan renderer (drawn in the
 * base class's render pass, or with dynamic rendering when the program draws
 * that way), the six baked fonts, the sound (the kit's mixer, its two sets of
 * recorded cues and its music, on the console's audio output), the pad read as
 * the kit's input frames, and the sample covers the designs show.
 *
 * prelude.h includes this file first, so a sample (compiled inside a namespace
 * of its own) reaches the kit's headers from here.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#pragma once

#include "vulkanexamplebase.h"
#include "ps5_samples.h"

#include "app/concept.hpp"
#include "app/shell.hpp"
#include "app/tour.hpp"
#include "audio/cues.hpp"
#include "audio/mixer.hpp"
#include "audio/music.hpp"
#include "concepts/concepts.hpp"
#include "core/input.hpp"
#include "core/save_file.hpp"
#include "core/settings.hpp"
#include "core/tween.hpp"
#include "demo/catalog.hpp"
#include "gfx/vk/vk_renderer.hpp"
#include "ui/components.hpp"
#include "ui/fonts.hpp"
#include "ui/glyphs.hpp"
#include "ui/widgets.hpp"

#include <sys/stat.h>

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace ps5ui {

// The screen of a title made with new-title.py --ui: its copy of one of the
// kit's designs, in examples/<program>/kit/screen.cpp (compiled outside the
// program's namespace, as every file in a program's kit/ folder is).
std::unique_ptr<hui::app::Concept> make_screen(hui::app::Context &context);

// The title's theme: the one picked on the start screen's Themes, kept in
// /app0/hui/theme.txt. It styles the start screen, the Samples menu and the
// samples' settings windows. A test run always has the default (tiles), so its
// pictures do not depend on what was picked.
const hui::ui::Theme &active_theme();
int active_theme_index();
void set_active_theme(int index);

// A row of controller hints (the DualSense glyph, then what it does) in a
// theme's colours: dark or light caps to suit its page, labels in its muted
// text. align: -1 starts at x, 0 centres on x, 1 ends at x. Returns the width.
float draw_hints(hui::gfx::DrawList &list, const hui::ui::Fonts &fonts, const hui::ui::Theme &theme,
	std::initializer_list<hui::ui::Hint> hints, float x, float cy, int align = -1, float size = 34.0f);

class Kit
{
public:
	struct Options {
		bool sound = true;      // the mixer on the console's output, and the recorded cues
		bool music = true;      // the kit's three songs, behind the cues
		bool covers = true;     // the sample catalogue's covers, which the designs show
		std::uint64_t seed = 0; // the music's shuffle
	};

	Kit() = default;
	Kit(const Kit &) = delete;
	Kit &operator=(const Kit &) = delete;
	~Kit();

	// assets is the folder of the kit's fonts and sounds (/app0/assets/hui)
	bool init(const hui::gfx::VkRendererConfig &config, const std::string &assets, const Options &options);
	// Stops the sound, then frees what init made (the device must still exist)
	void release();

	// The readings the frame's pad_poll took, folded into one input frame
	// (the base class's render loop polls before render())
	hui::InputFrame input();
	// Plays what an update asked for: each cue in its own sound set, or in
	// set when it names none; the rumble when haptics is on
	void play(const hui::ui::Feedback &feedback, hui::audio::SoundSet set, bool haptics = true);
	// Once a frame: ends a rumble whose time is up, keeps the music decoded ahead
	void tick(float dt);
	// Volumes and the confirm button from the kit's settings
	void apply(const hui::Settings &settings);
	// The controller's light bar (sent only when the colour changes)
	void light_bar(hui::gfx::Color color);

	std::string assets;
	hui::gfx::VkRenderer renderer;
	hui::ui::Fonts fonts;
	hui::demo::Catalog catalog;
	hui::audio::Mixer mixer;
	hui::audio::SoundBank sounds;
	hui::audio::MusicPlayer music;
	hui::InputTracker tracker;
	bool audio = false; // the console's output is playing the mixer

private:
	hui::gfx::Font faces_[6];
	std::vector<hui::PadSample> samples_;
	float rumble_left_ = 0.0f;
	std::uint32_t light_bar_ = 0xffffffffu;
	bool ready_ = false;
};

// Holding OPTIONS for a second leaves a kit program (a press stays the
// program's). While it is held, a plate at the top of the screen shows the
// OPTIONS glyph, a ring that fills, and where the hold goes.
class HoldToLeave
{
public:
	const char *label = "Back to the start screen";
	// true once OPTIONS has been held for the whole second
	bool update(float dt, bool held);
	void draw(hui::gfx::DrawList &list, const hui::ui::Fonts &fonts, const hui::ui::Theme &theme) const;
	float seconds = 1.0f;

private:
	float held_ = 0.0f;
};

// A program on the base class that draws with the kit
class KitExample : public VulkanExampleBase
{
public:
	Kit kit;
	~KitExample() override;

protected:
	// In prepare(), after VulkanExampleBase::prepare(): the kit draws in the
	// base class's render pass, or for the swapchain's formats when the
	// program uses dynamic rendering. A test run's music shuffle is seeded 0.
	void prepareKit(Kit::Options options = {});
	// Records drawCmdBuffers[currentBuffer] for the frame: offscreen(cmd)
	// (the program's own passes into its images, such as a 3D scene the kit
	// then draws with kit.renderer.import_texture), the kit's off-screen work,
	// then the pass on the swapchain image cleared to defaultClearColor with
	// scene(cmd) (the program's drawing under the UI, if any), the kit's
	// layers over it, and the base class's overlay when it shows. Queue the
	// kit's layers (kit.renderer.begin/backdrop/draw/glass, or a shell's
	// compose) before calling it.
	void buildKitCommandBuffer(const std::function<void(VkCommandBuffer)> &scene = nullptr,
		const std::function<void(VkCommandBuffer)> &offscreen = nullptr);
};

} // namespace ps5ui
