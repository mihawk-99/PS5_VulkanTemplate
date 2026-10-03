/*
 * UI over a game: the kit's HUD and pause menu over a 3D scene
 *
 * The starter's lit glTF lantern is drawn into an image of its own every frame,
 * and the kit (ps5/ui/kit.hpp) draws it as the bottom layer of its frame, so the
 * frosted HUD plates and the pause menu blur the scene itself. Over it: a health
 * bar, an objective tracker and toasts, and the pause menu behind OPTIONS. The
 * main pass uses dynamic rendering, so this sample also proves the kit's
 * pipelines for it (the gallery, uikit, proves the render pass).
 *
 * While the game runs the sticks turn and zoom the camera. OPTIONS pauses;
 * holding it for a second leaves. A test run takes a hit at one
 * second, completes an objective at one and a half, and pauses from two to four
 * seconds, so the pause menu runs and the last frame (five seconds) shows the
 * HUD over the scene, its objective plate frosted with the scene behind it.
 *
 * Copyright (C) 2026 Mihawk
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "kit.hpp"
#include "VulkanglTFModel.h"

class VulkanExample : public ps5ui::KitExample
{
public:
	vkglTF::Model model;
	float modelAngle{ 0.0f };
	bool spinModel{ true };

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 lightDirection;
		glm::vec4 cameraPosition;
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;
	VkPipeline pipeline{ VK_NULL_HANDLE };
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	// The scene's own images, at the display's size: the kit samples the colour
	struct Attachment {
		VkImage image{ VK_NULL_HANDLE };
		VkDeviceMemory memory{ VK_NULL_HANDLE };
		VkImageView view{ VK_NULL_HANDLE };
	} sceneColor, sceneDepth;
	const VkFormat sceneFormat{ VK_FORMAT_R8G8B8A8_UNORM };
	uint32_t sceneTexture{ 0 };

	// The kit's pieces and the lists they draw into
	hui::gfx::DrawList sceneList, hudList, overlayList;
	hui::ui::HealthBar health;
	hui::ui::ObjectiveTracker objectives;
	hui::ui::ToastStack toasts;
	hui::ui::PauseMenu pause;
	hui::ui::Feedback feedback;
	int lanternObjective{ -1 };
	float clock{ 0.0f };
	ps5ui::HoldToLeave leaving;
	hui::gfx::DrawList leavingList;

	enum Row { resumeRow, spinRow, hitRow, objectiveRow, healRow };

	VulkanExample() : KitExample()
	{
		title = "UI over a game";
		name = "uioverlay";
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -3.2f));
		camera.setRotation(glm::vec3(-12.0f, 30.0f, 0.0f));
		camera.setPerspective(50.0f, (float)width / (float)height, 0.05f, 64.0f);
		// The kit draws the interface and owns the pad
		settings.overlay = false;
		ps5.ownOverlay = true;
		// Dynamic rendering for the scene and the main pass, as the starter
		apiVersion = VK_API_VERSION_1_3;
		useDynamicRendering = true;
		requiresStencil = true;
		deviceCreatepNextChain = &baseDynamicRenderingFeatures;
	}

	~VulkanExample() override
	{
		if (device) {
			vkDeviceWaitIdle(device);
			kit.renderer.destroy_texture(sceneTexture);
			vkDestroyPipeline(device, pipeline, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto &buffer : uniformBuffers) {
				buffer.destroy();
			}
			for (Attachment *attachment : { &sceneColor, &sceneDepth }) {
				vkDestroyImageView(device, attachment->view, nullptr);
				vkDestroyImage(device, attachment->image, nullptr);
				vkFreeMemory(device, attachment->memory, nullptr);
			}
		}
	}

	void getEnabledFeatures() override
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
	}

	void createAttachment(Attachment &attachment, VkFormat format, VkImageUsageFlags usage, VkImageAspectFlags aspect)
	{
		VkImageCreateInfo imageCI = vks::initializers::imageCreateInfo();
		imageCI.imageType = VK_IMAGE_TYPE_2D;
		imageCI.format = format;
		imageCI.extent = { width, height, 1 };
		imageCI.mipLevels = 1;
		imageCI.arrayLayers = 1;
		imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCI.usage = usage;
		VK_CHECK_RESULT(vkCreateImage(device, &imageCI, nullptr, &attachment.image));
		VkMemoryRequirements requirements;
		vkGetImageMemoryRequirements(device, attachment.image, &requirements);
		VkMemoryAllocateInfo allocInfo = vks::initializers::memoryAllocateInfo();
		allocInfo.allocationSize = requirements.size;
		allocInfo.memoryTypeIndex = vulkanDevice->getMemoryType(requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		VK_CHECK_RESULT(vkAllocateMemory(device, &allocInfo, nullptr, &attachment.memory));
		VK_CHECK_RESULT(vkBindImageMemory(device, attachment.image, attachment.memory, 0));
		VkImageViewCreateInfo viewCI = vks::initializers::imageViewCreateInfo();
		viewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCI.format = format;
		viewCI.subresourceRange = { aspect, 0, 1, 0, 1 };
		viewCI.image = attachment.image;
		VK_CHECK_RESULT(vkCreateImageView(device, &viewCI, nullptr, &attachment.view));
	}

	VkImageAspectFlags depthAspect() const
	{
		return VK_IMAGE_ASPECT_DEPTH_BIT | (vks::tools::formatHasStencil(depthFormat) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);
	}

	void prepareScene()
	{
		// Khronos's Lantern (CC0), as the starter draws it (ps5/ASSETS.md)
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/ps5/lantern/lantern_starter.gltf", vulkanDevice, queue, glTFLoadingFlags);
		createAttachment(sceneColor, sceneFormat, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
		createAttachment(sceneDepth, depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthAspect());
		// The colour starts out readable: the kit may sample it before the first scene pass ends
		VkCommandBuffer cmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		vks::tools::insertImageMemoryBarrier(cmd, sceneColor.image, 0, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });
		vulkanDevice->flushCommandBuffer(cmd, queue, true);

		for (auto &buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		const std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0),
		};
		VkDescriptorSetLayoutCreateInfo descriptorLayout = vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
		VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout));
		VkDescriptorSetAllocateInfo allocInfo = vks::initializers::descriptorSetAllocateInfo(descriptorPool, &descriptorSetLayout, 1);
		for (size_t i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSets[i]));
			VkWriteDescriptorSet write = vks::initializers::writeDescriptorSet(descriptorSets[i], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBuffers[i].descriptor);
			vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
		}

		// The starter's pipeline, for the scene's own formats
		const std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayout, vkglTF::descriptorSetLayoutImage };
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
			loadShader(getShadersPath() + "uioverlay/model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "uioverlay/model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		VkPipelineRenderingCreateInfo renderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &sceneFormat,
			.depthAttachmentFormat = depthFormat,
			.stencilAttachmentFormat = vks::tools::formatHasStencil(depthFormat) ? depthFormat : VK_FORMAT_UNDEFINED,
		};
		VkGraphicsPipelineCreateInfo pipelineCI{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &renderingCreateInfo,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),
			.pVertexInputState = vkglTF::Vertex::getPipelineVertexInputState({ vkglTF::VertexComponent::Position, vkglTF::VertexComponent::Normal, vkglTF::VertexComponent::UV }),
			.pInputAssemblyState = &inputAssemblyState,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizationState,
			.pMultisampleState = &multisampleState,
			.pDepthStencilState = &depthStencilState,
			.pColorBlendState = &colorBlendState,
			.pDynamicState = &dynamicState,
			.layout = pipelineLayout,
		};
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipeline));
	}

	void prepareInterface()
	{
		const hui::ui::Theme theme = hui::ui::default_theme();
		health.style.theme = theme;
		health.title = "Warden";
		health.set_max(100.0f);
		health.set_value(100.0f, true);
		health.set_bounds({ 64.0f, 968.0f, 440.0f, 56.0f });
		objectives.style.theme = theme;
		objectives.style.backing = hui::ui::HudBacking::plate; // frosted: it blurs the scene
		objectives.title = "Objective";
		objectives.set_bounds({ 1440.0f, 64.0f, 416.0f, 190.0f });
		lanternObjective = objectives.add("Find the lantern");
		objectives.add("Light the hollow", "120 m");
		objectives.add("Reach the old bridge", "340 m");
		toasts.style.theme = theme;
		toasts.set_bounds({ 64.0f, 64.0f, 480.0f, 300.0f });
		pause.style.theme = theme;
		pause.title = "The lantern";
		pause.subtitle = "A 3D scene under the kit's HUD";
		std::vector<hui::ui::ListItem> rows(5);
		rows[resumeRow].title = "Resume";
		rows[spinRow].title = "Spin the lantern";
		rows[hitRow].title = "Take a hit";
		rows[objectiveRow].title = "Complete an objective";
		rows[healRow].title = "Heal";
		pause.set_items(rows);
		syncRows();
		pause.set_bounds({ 0.0f, 0.0f, hui::gfx::kVirtualWidth, hui::gfx::kVirtualHeight });
	}

	void syncRows()
	{
		pause.item(spinRow).value = spinModel ? "On" : "Off";
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		prepareKit({ .music = false });
		prepareScene();
		sceneTexture = kit.renderer.import_texture(sceneColor.view);
		if (sceneTexture == 0) {
			vks::tools::exitFatal("The kit could not take the scene's image", -1);
		}
		prepareInterface();
		prepared = true;
	}

	void takeHit()
	{
		health.set_value(std::max(0.0f, health.value() - 28.0f));
		feedback.play(hui::audio::Cue::error);
		feedback.rumble(0.7f, 0.12f);
	}

	void completeObjective()
	{
		if (lanternObjective >= 0 && objectives.complete(lanternObjective, &feedback)) {
			toasts.push(hui::ui::StatusKind::success, "Objective complete", "Find the lantern", 3.0f);
			lanternObjective = -1;
		}
	}

	// OPTIONS pauses; holding it for a second leaves (to the menu, or out of a title of one program)
	void holdOptions(float dt)
	{
		leaving.label = ps5.optionsEnds ? "Back to the menu" : "Close";
		if (!ps5.frameBudget && leaving.update(dt, (ps5_pad().held & PAD_OPTIONS) != 0)) {
			quit = true;
		}
	}

	void update(float dt)
	{
		feedback.clear();
		hui::InputFrame input = kit.input();
		if (ps5.frameBudget) {
			// A test run: the same three events at the same times, the pad ignored
			input = hui::InputFrame{};
			input.connected = true;
			const float before = clock;
			clock += dt;
			if (before < 1.0f && clock >= 1.0f) {
				takeHit();
			}
			if (before < 1.5f && clock >= 1.5f) {
				completeObjective();
			}
			if (before < 2.0f && clock >= 2.0f) {
				pause.open(feedback);
			}
			if (before < 4.0f && clock >= 4.0f) {
				pause.close(feedback);
			}
		}
		holdOptions(dt);
		if (pause.is_open()) {
			if (pause.handle(input, feedback) == hui::ui::Event::activated) {
				switch (pause.focus()) {
				case resumeRow: pause.close(feedback); break;
				case spinRow: spinModel = !spinModel; syncRows(); break;
				case hitRow: takeHit(); break;
				case objectiveRow: completeObjective(); break;
				case healRow: health.set_value(health.max()); feedback.play(hui::audio::Cue::complete); break;
				}
			}
		} else {
			if (input.is_pressed(hui::Action::menu)) {
				pause.open(feedback);
			}
			// The sticks turn and zoom the camera while the game runs
			camera.rotate(glm::vec3(-input.stick_y, input.stick_x, 0.0f) * 90.0f * dt);
			const float distance = std::max(1.0f, std::abs(camera.position.z));
			camera.translate(glm::vec3(0.0f, 0.0f, -input.stick2_y * distance * dt));
			if (spinModel) {
				modelAngle += 0.4f * dt;
			}
		}
		health.update(dt);
		objectives.update(dt);
		toasts.update(dt, feedback);
		pause.update(dt);
		kit.play(feedback, hui::audio::SoundSet::glass);
		kit.tick(dt);
	}

	void compose()
	{
		// The scene, then its blurred copy, then the HUD and the pause menu over both
		sceneList.clear();
		sceneList.image(sceneTexture, { 0.0f, 0.0f, hui::gfx::kVirtualWidth, hui::gfx::kVirtualHeight }, hui::gfx::kFullUv, hui::gfx::Color::rgb(0xffffff));
		const uint32_t glass = kit.renderer.glass_texture();
		hudList.clear();
		hui::ui::Canvas hud{ hudList, kit.fonts, glass, clock };
		health.draw(hud);
		objectives.draw(hud);
		toasts.draw(hud);
		overlayList.clear();
		hui::ui::Canvas overlay{ overlayList, kit.fonts, glass, clock };
		pause.draw(overlay);
		kit.renderer.begin();
		kit.renderer.draw(sceneList);
		kit.renderer.glass();
		kit.renderer.draw(hudList);
		kit.renderer.draw(overlayList);
		leavingList.clear();
		leaving.draw(leavingList, kit.fonts, ps5ui::active_theme());
		kit.renderer.draw(leavingList);
	}

	void drawScene(VkCommandBuffer cmd)
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		uniformData.model = glm::rotate(glm::mat4(1.0f), modelAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		uniformData.lightDirection = glm::vec4(glm::normalize(glm::vec3(-0.45f, 0.75f, -0.5f)), 0.0f);
		uniformData.cameraPosition = glm::inverse(camera.matrices.view)[3];
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));

		// The frame before may still be sampling the colour: wait for its reads
		const VkImageSubresourceRange colorRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		const VkImageSubresourceRange depthRange{ depthAspect(), 0, 1, 0, 1 };
		vks::tools::insertImageMemoryBarrier(cmd, sceneColor.image, 0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, colorRange);
		vks::tools::insertImageMemoryBarrier(cmd, sceneDepth.image, 0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, depthRange);
		VkRenderingAttachmentInfo colorAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = sceneColor.view,
			.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.clearValue = { .color = { { 0.05f, 0.06f, 0.09f, 1.0f } } },
		};
		VkRenderingAttachmentInfo depthAttachment{
			.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
			.imageView = sceneDepth.view,
			.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.clearValue = { .depthStencil = { 1.0f, 0 } },
		};
		const bool stencil = vks::tools::formatHasStencil(depthFormat);
		VkRenderingInfo renderingInfo{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
			.renderArea = { { 0, 0 }, { width, height } },
			.layerCount = 1,
			.colorAttachmentCount = 1,
			.pColorAttachments = &colorAttachment,
			.pDepthAttachment = &depthAttachment,
			.pStencilAttachment = stencil ? &depthAttachment : nullptr,
		};
		vkCmdBeginRendering(cmd, &renderingInfo);
		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmd, 0, 1, &viewport);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmd, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		model.draw(cmd, vkglTF::RenderFlags::BindImages, pipelineLayout);
		vkCmdEndRendering(cmd);
		// Ready for the kit: the glass copy and the bottom layer read it
		vks::tools::insertImageMemoryBarrier(cmd, sceneColor.image, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, colorRange);
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		prepareFrame();
		update(std::min(frameTimer, 0.05f));
		compose();
		buildKitCommandBuffer(nullptr, [this](VkCommandBuffer cmd) { drawScene(cmd); });
		submitFrame();
	}
};

VULKAN_EXAMPLE_MAIN()
