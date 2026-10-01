/*
 * PS5 Vulkan Template - the base class's console parts: the pad and the screenshot.
 *
 * VulkanExampleBase calls these from its PS5 render loop and submitFrame
 * (base/vulkanexamplebase.cpp, VK_EXAMPLE_PS5).
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "ps5_samples.h"

#include <sys/stat.h>

/* One pad state for the whole title, so a button held while one sample ends
 * and the next starts is not seen as a new press by the next. */
struct pad &ps5_pad()
{
	static struct pad pad{};
	return pad;
}

void VulkanExampleBase::ps5HandleInput()
{
	struct pad &pad = ps5_pad();
	pad_poll(&pad);
	// No imgui.ini: window positions are not kept between runs
	ImGui::GetIO().IniFilename = nullptr;
	if (ps5.frameBudget) {
		const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
		if (ps5.framesDrawn == ps5.frameBudget / 2) {
			ps5.halfwayTime = now;
		}
		if (ps5.framesDrawn + 1 == ps5.frameBudget) {
			ps5.lastFrameTime = now;
		}
		// A test run is the same whoever holds the pad
		return;
	}

	// OPTIONS ends the sample and goes back to the launcher's menu. In a title
	// with one program there is no menu to go back to: it shows and hides the
	// settings, as the touch pad's button does (F1 on a desktop)
	if ((pad.pressed & PAD_OPTIONS) && !ps5.ownOverlay && ps5.optionsEnds) {
		quit = true;
	}
	if ((pad.pressed & (ps5.optionsEnds ? PAD_TOUCH_PAD : PAD_TOUCH_PAD | PAD_OPTIONS)) && !ps5.ownOverlay) {
		ui.visible = !ui.visible;
	}

	// The overlay's widgets are reached with the D-pad: cross acts, circle cancels,
	// L1 and R1 change windows, L2 and R2 change a slider slower or faster. The
	// current context is the overlay's, or the one a sample made itself (imgui)
	ImGuiIO &io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
	io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
	memset(io.NavInputs, 0, sizeof(io.NavInputs));
	if (ui.visible) {
		const auto held = [&](uint32_t button) { return (pad.held & button) ? 1.0f : 0.0f; };
		io.NavInputs[ImGuiNavInput_Activate] = held(PAD_CROSS);
		io.NavInputs[ImGuiNavInput_Cancel] = held(PAD_CIRCLE);
		io.NavInputs[ImGuiNavInput_Input] = held(PAD_TRIANGLE);
		io.NavInputs[ImGuiNavInput_Menu] = held(PAD_SQUARE);
		io.NavInputs[ImGuiNavInput_DpadLeft] = held(PAD_LEFT);
		io.NavInputs[ImGuiNavInput_DpadRight] = held(PAD_RIGHT);
		io.NavInputs[ImGuiNavInput_DpadUp] = held(PAD_UP);
		io.NavInputs[ImGuiNavInput_DpadDown] = held(PAD_DOWN);
		io.NavInputs[ImGuiNavInput_FocusPrev] = held(PAD_L1);
		io.NavInputs[ImGuiNavInput_FocusNext] = held(PAD_R1);
		io.NavInputs[ImGuiNavInput_TweakSlow] = pad.l2;
		io.NavInputs[ImGuiNavInput_TweakFast] = pad.r2;
	}

	// The sticks move the camera: the left one turns it (or walks, for a
	// first-person camera), the right one zooms (or looks around)
	gamePadState.axisLeft = glm::vec2(pad.left_x, pad.left_y);
	gamePadState.axisRight = glm::vec2(pad.right_x, pad.right_y);
	if (ps5.ownOverlay) {
		return;
	}
	if (camera.type == Camera::CameraType::firstperson) {
		camera.updatePad(gamePadState.axisLeft, gamePadState.axisRight, frameTimer);
		return;
	}
	const float degreesPerSecond = 90.0f;
	if (pad.left_x != 0.0f || pad.left_y != 0.0f) {
		camera.rotate(glm::vec3(-pad.left_y, pad.left_x, 0.0f) * degreesPerSecond * frameTimer);
	}
	if (pad.right_y != 0.0f) {
		// In proportion to the distance, so a close camera and a far one zoom alike
		const float distance = std::max(1.0f, std::abs(camera.position.z));
		camera.translate(glm::vec3(0.0f, 0.0f, -pad.right_y * distance * frameTimer));
	}
}

VkSemaphore VulkanExampleBase::ps5CaptureSwapchainImage(VkSemaphore renderComplete)
{
	if (ps5CaptureComplete == VK_NULL_HANDLE) {
		VkSemaphoreCreateInfo semaphoreCI{ .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &ps5CaptureComplete));
	}

	// A cached host buffer when there is one: reading 32 MB back from uncached memory is slow
	VkMemoryPropertyFlags hostFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
		const VkMemoryPropertyFlags flags = deviceMemoryProperties.memoryTypes[i].propertyFlags;
		if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)) {
			hostFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			break;
		}
	}
	vks::Buffer readback;
	const VkDeviceSize size = (VkDeviceSize)width * height * 4;
	VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_TRANSFER_DST_BIT, hostFlags, &readback, size));

	VkCommandBuffer cmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	const VkImage image = swapChain.images[currentImageIndex];
	const VkImageSubresourceRange range{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	vks::tools::insertImageMemoryBarrier(cmd, image, 0, VK_ACCESS_TRANSFER_READ_BIT,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, range);
	const VkBufferImageCopy region{
		.imageSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 },
		.imageExtent = { width, height, 1 },
	};
	vkCmdCopyImageToBuffer(cmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, readback.buffer, 1, &region);
	vks::tools::insertImageMemoryBarrier(cmd, image, VK_ACCESS_TRANSFER_READ_BIT, 0,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, range);
	VK_CHECK_RESULT(vkEndCommandBuffer(cmd));
	const VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	const VkSubmitInfo submitInfo{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &renderComplete,
		.pWaitDstStageMask = &waitStage,
		.commandBufferCount = 1,
		.pCommandBuffers = &cmd,
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &ps5CaptureComplete,
	};
	VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	VK_CHECK_RESULT(vkQueueWaitIdle(queue));
	vkFreeCommandBuffers(device, vulkanDevice->commandPool, 1, &cmd);

	// Half the display's size, each pixel the mean of four: a 1920x1080 picture
	// is enough to judge a frame and fetches four times faster than the full one
	VK_CHECK_RESULT(readback.map());
	VK_CHECK_RESULT(readback.invalidate());
	const bool bgr = swapChain.colorFormat == VK_FORMAT_B8G8R8A8_UNORM || swapChain.colorFormat == VK_FORMAT_B8G8R8A8_SRGB;
	const uint32_t outWidth = width / 2, outHeight = height / 2;
	std::vector<uint8_t> rgb((size_t)outWidth * outHeight * 3);
	const uint8_t *pixels = static_cast<const uint8_t *>(readback.mapped);
	for (uint32_t y = 0; y < outHeight; y++) {
		for (uint32_t x = 0; x < outWidth; x++) {
			uint32_t sum[3] = { 0, 0, 0 };
			for (uint32_t dy = 0; dy < 2; dy++) {
				for (uint32_t dx = 0; dx < 2; dx++) {
					const uint8_t *p = pixels + (((size_t)(y * 2 + dy) * width) + (x * 2 + dx)) * 4;
					for (int c = 0; c < 3; c++) {
						sum[c] += p[bgr ? 2 - c : c];
					}
				}
			}
			for (int c = 0; c < 3; c++) {
				rgb[((size_t)y * outWidth + x) * 3 + c] = (uint8_t)((sum[c] + 2) / 4);
			}
		}
	}
	readback.destroy();

	FILE *file = fopen(ps5.screenshotPath.c_str(), "wb");
	if (file) {
		fprintf(file, "P6\n%u %u\n255\n", outWidth, outHeight);
		fwrite(rgb.data(), 1, rgb.size(), file);
		fclose(file);
		// Reachable over FTP: the console's FTP server is another process
		chmod(ps5.screenshotPath.c_str(), 0666);
		say("screenshot: %s", ps5.screenshotPath.c_str());
	} else {
		say("screenshot: could not write %s", ps5.screenshotPath.c_str());
	}
	return ps5CaptureComplete;
}
