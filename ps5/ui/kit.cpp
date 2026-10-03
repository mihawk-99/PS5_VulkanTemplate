/*
 * PS5 Vulkan Template - the UI module: PS5_VKHomebrewUI's kit on the base class.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"

#include "platform.h"

#include "core/save_file.hpp"

#include <span>

namespace ps5ui {

namespace {

// The faces in ui::Fonts order, as the kit bakes them
const char *const faceFiles[6] = {
	"inter-regular.huifont", "inter-semibold.huifont", "montserrat-medium.huifont",
	"dejavu-sans-mono.huifont", "press-start-2p.huifont", "patrick-hand.huifont",
};

// The audio thread renders the mixer; posting to it never blocks the frame
void fillFromMixer(int16_t *frames, int count, void *user)
{
	static_cast<hui::audio::Mixer *>(user)->render(frames, count);
}

} // namespace

Kit::~Kit()
{
	release();
}

bool Kit::init(const hui::gfx::VkRendererConfig &config, const std::string &assets, const Options &options)
{
	this->assets = assets;
	if (!renderer.init(config)) {
		say("ui kit: renderer failed, VkResult %d", (int)renderer.last_error());
		return false;
	}
	ready_ = true;
	hui::ui::FontRef *refs[6] = { &fonts.regular, &fonts.semibold, &fonts.display, &fonts.mono, &fonts.pixel, &fonts.hand };
	for (int i = 0; i < 6; i++) {
		std::string data;
		const std::string path = assets + "/fonts/" + faceFiles[i];
		if (!hui::save::read_file(path, &data) || !faces_[i].load(data)) {
			say("ui kit: font %s: %s", path.c_str(), faces_[i].error().c_str());
			return false;
		}
		refs[i]->font = &faces_[i];
		refs[i]->texture = renderer.create_font_texture(faces_[i]);
		if (refs[i]->texture == 0) {
			say("ui kit: font %s: no texture, VkResult %d", faceFiles[i], (int)renderer.last_error());
			return false;
		}
	}
	if (options.covers && !catalog.build_covers(renderer, fonts)) {
		say("ui kit: cover art failed, VkResult %d", (int)renderer.last_error());
		return false;
	}
	if (options.sound) {
		// The music attaches to the mixer before the audio thread starts
		const int songs = options.music ? music.init(mixer, assets + "/audio/music", options.seed) : 0;
		const auto bank = sounds.load(assets + "/audio/sfx");
		audio = audio_start(fillFromMixer, &mixer);
		say("ui kit: %d songs, %d sounds (%d rejected), output %s", songs, bank.files, bank.rejected,
			audio ? "playing" : "none");
	}
	return true;
}

void Kit::release()
{
	if (audio) {
		audio_stop();
		audio = false;
	}
	if (rumble_left_ > 0.0f) {
		pad_vibrate(0.0f, 0.0f);
		rumble_left_ = 0.0f;
	}
	if (ready_) {
		renderer.release();
		ready_ = false;
	}
}

hui::InputFrame Kit::input()
{
	const pad_reading *readings = nullptr;
	const int count = pad_readings(&readings);
	samples_.resize(count);
	for (int i = 0; i < count; i++) {
		const pad_reading &r = readings[i];
		samples_[i] = { r.buttons, r.left_x, r.left_y, r.right_x, r.right_y, r.l2, r.r2, r.connected, r.timestamp_us };
	}
	return tracker.update(std::span<const hui::PadSample>(samples_), (std::uint64_t)(now_seconds() * 1e6));
}

void Kit::play(const hui::ui::Feedback &feedback, hui::audio::SoundSet set, bool haptics)
{
	for (const hui::audio::CueEvent &event : feedback.cues) {
		sounds.play(mixer, event.set == hui::audio::SoundSet::count ? set : event.set, event);
		if (event.cue == hui::audio::Cue::complete || event.cue == hui::audio::Cue::welcome) {
			music.duck();
		}
	}
	if (haptics && feedback.rumble_strength > 0.0f && feedback.rumble_seconds > 0.0f) {
		// The small motor gives the crisp tick a UI wants; the large one adds
		// weight only to strong effects (the kit's own pad does the same)
		const float strength = std::min(feedback.rumble_strength, 1.0f);
		pad_vibrate(strength > 0.6f ? (strength - 0.6f) * 2.5f : 0.0f, strength);
		rumble_left_ = feedback.rumble_seconds;
	}
}

void Kit::tick(float dt)
{
	if (rumble_left_ > 0.0f) {
		rumble_left_ -= dt;
		if (rumble_left_ <= 0.0f) {
			pad_vibrate(0.0f, 0.0f);
		}
	}
	music.pump(dt);
}

void Kit::apply(const hui::Settings &settings)
{
	mixer.set_bus_gain(hui::audio::Bus::music, hui::Settings::gain(settings.music_volume));
	mixer.set_bus_gain(hui::audio::Bus::sfx, hui::Settings::gain(settings.sfx_volume));
	mixer.set_bus_gain(hui::audio::Bus::ui, hui::Settings::gain(settings.ui_volume));
	hui::InputSettings input = tracker.settings();
	input.swap_confirm = settings.swap_confirm;
	tracker.set_settings(input);
}

void Kit::light_bar(hui::gfx::Color color)
{
	const auto channel = [](float value) { return (uint8_t)(std::clamp(value, 0.0f, 1.0f) * 255.0f); };
	const uint8_t r = channel(color.r), g = channel(color.g), b = channel(color.b);
	const uint32_t packed = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
	if (packed != light_bar_) {
		light_bar_ = packed;
		pad_light_bar(r, g, b);
	}
}

KitExample::~KitExample()
{
	// Before the base class destroys the device
	kit.release();
}

void KitExample::prepareKit(Kit::Options options)
{
	hui::gfx::VkRendererConfig config;
	config.physical_device = physicalDevice;
	config.device = device;
	config.queue = queue;
	config.queue_family = vulkanDevice->queueFamilyIndices.graphics;
	config.pipeline_cache = pipelineCache;
	config.frames_in_flight = maxConcurrentFrames;
	if (useDynamicRendering) {
		config.color_format = swapChain.colorFormat;
		config.depth_format = depthFormat;
		config.stencil_format = vks::tools::formatHasStencil(depthFormat) ? depthFormat : VK_FORMAT_UNDEFINED;
	} else {
		config.render_pass = renderPass;
	}
	if (benchmark.active) {
		options.seed = 0;
	} else if (options.seed == 0) {
		options.seed = (std::uint64_t)(now_seconds() * 1e6);
	}
	if (!kit.init(config, getAssetPath() + "hui", options)) {
		vks::tools::exitFatal("The UI kit could not start (klog has why)", -1);
	}
}

void KitExample::buildKitCommandBuffer(const std::function<void(VkCommandBuffer)> &scene,
	const std::function<void(VkCommandBuffer)> &offscreen)
{
	VkCommandBuffer cmd = drawCmdBuffers[currentBuffer];
	VkCommandBufferBeginInfo beginInfo = vks::initializers::commandBufferBeginInfo();
	VK_CHECK_RESULT(vkBeginCommandBuffer(cmd, &beginInfo));
	// The program's own passes first: the glass copies may replay what they drew
	if (offscreen) {
		offscreen(cmd);
	}
	// Uploads the frame's shapes and makes the glass copies, outside the pass
	kit.renderer.prepare(cmd, currentBuffer);
	if (useDynamicRendering) {
		beginDynamicRendering(cmd);
	} else {
		VkClearValue clearValues[2]{};
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo passInfo = vks::initializers::renderPassBeginInfo();
		passInfo.renderPass = renderPass;
		passInfo.framebuffer = frameBuffers[currentImageIndex];
		passInfo.renderArea.extent = { width, height };
		passInfo.clearValueCount = 2;
		passInfo.pClearValues = clearValues;
		vkCmdBeginRenderPass(cmd, &passInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	if (scene) {
		scene(cmd);
	}
	kit.renderer.draw(cmd, (int)width, (int)height);
	drawUI(cmd);
	if (useDynamicRendering) {
		endDynamicRendering(cmd);
	} else {
		vkCmdEndRenderPass(cmd);
	}
	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
}

} // namespace ps5ui
