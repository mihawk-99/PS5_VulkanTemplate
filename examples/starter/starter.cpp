/*
 * Starter - the program a new PS5 homebrew grows from.
 *
 * A textured, lit glTF model from the title folder, drawn with dynamic
 * rendering into the swapchain image and the base class's depth buffer. The
 * base class does the rest: the display, the frames, the camera on the pad's
 * sticks, and the settings window, moved through with the D-pad (ps5/README.md).
 * ps5/tools/new-title.py makes a new title from the samples' foundation with
 * this file as its program: replace the model, and grow the class.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */

#include "vulkanexamplebase.h"
#include "VulkanglTFModel.h"

class VulkanExample : public VulkanExampleBase
{
public:
	vkglTF::Model model;

	// The settings window's choices
	bool spinModel{ true };
	bool moveLight{ false };
	bool wireframe{ false };
	float modelAngle{ 0.0f };
	float lightAngle{ 0.0f };

	struct UniformData {
		glm::mat4 projection;
		glm::mat4 view;
		glm::mat4 model;
		glm::vec4 lightDirection;
		glm::vec4 cameraPosition;
	} uniformData;
	std::array<vks::Buffer, maxConcurrentFrames> uniformBuffers;

	struct {
		VkPipeline solid{ VK_NULL_HANDLE };
		VkPipeline wireframe{ VK_NULL_HANDLE };
	} pipelines;
	VkPipelineLayout pipelineLayout{ VK_NULL_HANDLE };
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	std::array<VkDescriptorSet, maxConcurrentFrames> descriptorSets{};

	VulkanExample() : VulkanExampleBase()
	{
		title = "Starter";
		resetCamera();
		// Vulkan 1.3's dynamic rendering: no render pass or framebuffer objects.
		// The base class's helpers (beginDynamicRendering) transition the depth
		// buffer's stencil too, so it asks for a depth and stencil format.
		apiVersion = VK_API_VERSION_1_3;
		useDynamicRendering = true;
		requiresStencil = true;
		deviceCreatepNextChain = &baseDynamicRenderingFeatures;
	}

	~VulkanExample()
	{
		if (device) {
			vkDestroyPipeline(device, pipelines.solid, nullptr);
			vkDestroyPipeline(device, pipelines.wireframe, nullptr);
			vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
			for (auto& buffer : uniformBuffers) {
				buffer.destroy();
			}
		}
	}

	void resetCamera()
	{
		camera.type = Camera::CameraType::lookat;
		camera.setPosition(glm::vec3(0.0f, 0.0f, -3.2f));
		camera.setRotation(glm::vec3(-12.0f, 30.0f, 0.0f));
		camera.setPerspective(50.0f, (float)width / (float)height, 0.05f, 64.0f);
	}

	void getEnabledFeatures() override
	{
		if (deviceFeatures.samplerAnisotropy) {
			enabledFeatures.samplerAnisotropy = VK_TRUE;
		}
		// Wireframe needs non-solid fill modes; the setting is offered only with them
		if (deviceFeatures.fillModeNonSolid) {
			enabledFeatures.fillModeNonSolid = VK_TRUE;
		}
	}

	void loadAssets()
	{
		// Khronos's Lantern (CC0), 1.6 tall and centred (ps5/ASSETS.md): replace it with yours
		const uint32_t glTFLoadingFlags = vkglTF::FileLoadingFlags::PreTransformVertices | vkglTF::FileLoadingFlags::FlipY;
		model.loadFromFile(getAssetPath() + "models/ps5/lantern/lantern_starter.gltf", vulkanDevice, queue, glTFLoadingFlags);
	}

	void setupDescriptors()
	{
		std::vector<VkDescriptorPoolSize> poolSizes = {
			vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxConcurrentFrames),
		};
		VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxConcurrentFrames);
		VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));
		// Set 0: the frame's uniforms. Set 1 is the model's base colour texture (vkglTF's layout)
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
	}

	void preparePipelines()
	{
		const std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayout, vkglTF::descriptorSetLayoutImage };
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = vks::initializers::pipelineLayoutCreateInfo(setLayouts.data(), static_cast<uint32_t>(setLayouts.size()));
		VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

		VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
		// No culling: glTF models are often open, and the shader lights both sides
		VkPipelineRasterizationStateCreateInfo rasterizationState = vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
		VkPipelineColorBlendAttachmentState blendAttachmentState = vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
		VkPipelineColorBlendStateCreateInfo colorBlendState = vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
		VkPipelineDepthStencilStateCreateInfo depthStencilState = vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
		VkPipelineViewportStateCreateInfo viewportState = vks::initializers::pipelineViewportStateCreateInfo(1, 1, 0);
		VkPipelineMultisampleStateCreateInfo multisampleState = vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
		std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicState = vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
			loadShader(getShadersPath() + "starter/model.vert.spv", VK_SHADER_STAGE_VERTEX_BIT),
			loadShader(getShadersPath() + "starter/model.frag.spv", VK_SHADER_STAGE_FRAGMENT_BIT),
		};
		// With dynamic rendering the pipeline names its attachments' formats instead of a render pass
		VkPipelineRenderingCreateInfo renderingCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
			.colorAttachmentCount = 1,
			.pColorAttachmentFormats = &swapChain.colorFormat,
			.depthAttachmentFormat = depthFormat,
			.stencilAttachmentFormat = depthFormat,
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
		VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.solid));
		if (enabledFeatures.fillModeNonSolid) {
			rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
			VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCI, nullptr, &pipelines.wireframe));
		}
	}

	void prepareUniformBuffers()
	{
		for (auto& buffer : uniformBuffers) {
			VK_CHECK_RESULT(vulkanDevice->createBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffer, sizeof(UniformData), &uniformData));
			VK_CHECK_RESULT(buffer.map());
		}
	}

	void updateUniformBuffers()
	{
		uniformData.projection = camera.matrices.perspective;
		uniformData.view = camera.matrices.view;
		uniformData.model = glm::rotate(glm::mat4(1.0f), modelAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		// The light comes from above, left and in front; the base class's world has Y down
		const glm::mat4 lightTurn = glm::rotate(glm::mat4(1.0f), lightAngle, glm::vec3(0.0f, 1.0f, 0.0f));
		uniformData.lightDirection = lightTurn * glm::vec4(glm::normalize(glm::vec3(-0.45f, 0.75f, -0.5f)), 0.0f);
		uniformData.cameraPosition = glm::inverse(camera.matrices.view)[3];
		memcpy(uniformBuffers[currentBuffer].mapped, &uniformData, sizeof(uniformData));
	}

	void prepare() override
	{
		VulkanExampleBase::prepare();
		loadAssets();
		prepareUniformBuffers();
		setupDescriptors();
		preparePipelines();
		prepared = true;
	}

	void buildCommandBuffer()
	{
		VkCommandBuffer cmdBuffer = drawCmdBuffers[currentBuffer];
		VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
		VK_CHECK_RESULT(vkBeginCommandBuffer(cmdBuffer, &cmdBufInfo));
		// Clears the swapchain image and the depth buffer, with the barriers they need
		beginDynamicRendering(cmdBuffer);
		VkViewport viewport = vks::initializers::viewport((float)width, (float)height, 0.0f, 1.0f);
		vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
		VkRect2D scissor = vks::initializers::rect2D(width, height, 0, 0);
		vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);
		vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentBuffer], 0, nullptr);
		vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, wireframe ? pipelines.wireframe : pipelines.solid);
		// Each material's base colour texture is bound as set 1
		model.draw(cmdBuffer, vkglTF::RenderFlags::BindImages, pipelineLayout);
		drawUI(cmdBuffer);
		endDynamicRendering(cmdBuffer);
		VK_CHECK_RESULT(vkEndCommandBuffer(cmdBuffer));
	}

	void render() override
	{
		if (!prepared) {
			return;
		}
		VulkanExampleBase::prepareFrame();
		if (spinModel) {
			modelAngle += 0.4f * frameTimer;
		}
		if (moveLight) {
			lightAngle += 0.8f * frameTimer;
		}
		updateUniformBuffers();
		buildCommandBuffer();
		VulkanExampleBase::submitFrame();
	}

	void OnUpdateUIOverlay(vks::UIOverlay *overlay) override
	{
		if (overlay->header("Settings")) {
			overlay->checkBox("Spin the model", &spinModel);
			overlay->checkBox("Move the light", &moveLight);
			if (enabledFeatures.fillModeNonSolid) {
				overlay->checkBox("Wireframe", &wireframe);
			}
			if (overlay->button("Reset the camera")) {
				resetCamera();
			}
			if (overlay->button("Quit")) {
				quit = true;
			}
		}
	}
};

VULKAN_EXAMPLE_MAIN()
