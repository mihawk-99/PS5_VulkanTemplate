/*
 * PS5 Vulkan Samples - the menu.
 *
 * The menu is itself an example on the base class: it brings Vulkan up, lists
 * the samples proven on the console in the overlay, and ends when one is
 * chosen, so each sample then starts on a device of its own.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "ps5_samples.h"

namespace {

const ImVec4 accent{ 0.98f, 0.36f, 0.22f, 1.0f };
const ImVec4 dim{ 0.62f, 0.65f, 0.72f, 1.0f };
const ImVec4 failure{ 1.0f, 0.42f, 0.38f, 1.0f };

class Launcher : public VulkanExampleBase
{
public:
	int chosen{ -2 }; // -2: still choosing, -1: Quit, else an index in ps5Samples
	int selected;
	std::string message;
	std::vector<int> entries;
	bool focusSet{ false };

	Launcher(int selected, const std::string &message) : selected(selected), message(message)
	{
		title = PS5_TITLE_NAME;
		name = "ps5vulkansamples";
		ps5.ownOverlay = true;
		defaultClearColor = { { 0.035f, 0.04f, 0.06f, 1.0f } };
		for (size_t i = 0; i < ps5SampleCount; i++) {
			if (ps5Samples[i].inMenu) {
				entries.push_back((int)i);
			}
		}
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepared = true;
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		prepareFrame();
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		VkClearValue clearValues[2]{};
		clearValues[0].color = defaultClearColor;
		clearValues[1].depthStencil = { 1.0f, 0 };
		VkRenderPassBeginInfo renderPassBeginInfo{
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = renderPass,
			.framebuffer = frameBuffers[currentImageIndex],
			.renderArea = { .offset = { 0, 0 }, .extent = { width, height } },
			.clearValueCount = 2,
			.pClearValues = clearValues,
		};
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		vkCmdBeginRenderPass(cmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
		drawUI(cmdBuffer);
		vkCmdEndRenderPass(cmdBuffer);
		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
		submitFrame();
		if (chosen != -2) {
			quit = true;
		}
	}

	void OnUpdateUIOverlay(vks::UIOverlay *overlay) override
	{
		const float s = overlay->scale;
		ImGui::SetNextWindowPos(ImVec2(width * 0.5f, height * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		ImGui::SetNextWindowSize(ImVec2(1100.0f * s, 0.0f), ImGuiCond_Always);
		if (!focusSet) {
			ImGui::SetNextWindowFocus();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f * s);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(28.0f * s, 24.0f * s));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f * s, 6.0f * s));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.08f, 0.11f, 0.96f));
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(accent.x, accent.y, accent.z, 0.30f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(accent.x, accent.y, accent.z, 0.45f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(accent.x, accent.y, accent.z, 0.60f));
		ImGui::Begin(PS5_TITLE_NAME, nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings);

		ImGui::SetWindowFontScale(1.6f);
		ImGui::TextColored(accent, "%s", PS5_TITLE_NAME);
		ImGui::SetWindowFontScale(1.0f);
		ImGui::TextColored(dim, "Vulkan on RADV, on the console");
		ImGui::TextColored(dim, "%s, Vulkan %u.%u.%u, %ux%u", deviceProperties.deviceName,
			VK_API_VERSION_MAJOR(deviceProperties.apiVersion), VK_API_VERSION_MINOR(deviceProperties.apiVersion),
			VK_API_VERSION_PATCH(deviceProperties.apiVersion), width, height);
		ImGui::Separator();
		ImGui::Spacing();

		if (entries.empty()) {
			ImGui::TextColored(dim, "No sample has been proven on the console yet.");
		}
		for (int index : entries) {
			const Ps5Sample &sample = ps5Samples[index];
			ImGui::PushID(index);
			if (ImGui::Selectable(sample.title, index == selected)) {
				chosen = index;
			}
			if (!focusSet && index == selected) {
				ImGui::SetItemDefaultFocus();
			}
			if (ImGui::IsItemFocused()) {
				selected = index;
			}
			ImGui::Indent(24.0f * s);
			ImGui::TextColored(dim, "%s", sample.description);
			ImGui::Unindent(24.0f * s);
			ImGui::PopID();
		}
		ImGui::Spacing();
		if (ImGui::Selectable("Quit", false)) {
			chosen = -1;
		}
		if (!focusSet && (selected < 0 || entries.empty())) {
			ImGui::SetItemDefaultFocus();
		}
		focusSet = true;

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextColored(dim, "CROSS start.  In a sample: OPTIONS back here, the sticks move the camera,");
		ImGui::TextColored(dim, "the D-pad and CROSS change its settings, TOUCH PAD hides them.");
		if (!message.empty()) {
			ImGui::Spacing();
			ImGui::PushTextWrapPos(0.0f);
			ImGui::TextColored(failure, "%s", message.c_str());
			ImGui::PopTextWrapPos();
		}
		ImGui::End();
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(3);
	}
};

} // namespace

int ps5_run_launcher(int selected, const std::string &message, uint32_t frameBudget,
	const std::string &screenshotPath)
{
	Launcher *launcher = new Launcher(selected, message);
	launcher->ps5.frameBudget = frameBudget;
	launcher->ps5.screenshotPath = screenshotPath;
	int chosen = -1;
	try {
		launcher->initVulkan();
		launcher->prepare();
		launcher->renderLoop();
		chosen = frameBudget ? -1 : launcher->chosen;
	} catch (const std::exception &e) {
		say("menu: %s", e.what());
	}
	if (launcher->vulkanDevice) {
		vkDeviceWaitIdle(launcher->vulkanDevice->logicalDevice);
	}
	delete launcher;
	return chosen;
}
