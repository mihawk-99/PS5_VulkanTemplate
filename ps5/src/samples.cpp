/*
 * PS5 Vulkan Samples - the samples linked into the title.
 *
 * One line a sample: SAMPLE(id, in the menu, title, description). The build
 * (ps5/CMakeLists.txt) compiles examples/<id>/<id>.cpp for every line here, in
 * a namespace of its own, and packages shaders/glsl/<id>/. A sample goes in the
 * menu only once it has been proven on the console (ps5/README.md); until then
 * test runs reach it and the menu does not.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
#include "ps5_samples.h"

#define PS5_SAMPLES                                                                                          \
	SAMPLE(gltfloading, false, "glTF model loading", "A glTF scene: meshes, materials, textures, node hierarchy") \
	SAMPLE(texturemipmapgen, false, "Mipmaps made at run time", "A texture's mip chain made with image blits, and how filtering uses it") \
	SAMPLE(pbrtexture, false, "Physically based rendering", "Metal and roughness maps lit by an HDR environment (image based lighting)") \
	SAMPLE(shadowmapping, false, "Shadow mapping", "A directional light's depth map, filtered with PCF") \
	SAMPLE(deferred, false, "Deferred shading", "A G-buffer in several render targets, then many lights in one pass") \
	SAMPLE(bloom, false, "Bloom", "Bright parts blurred in two separable passes and added back") \
	SAMPLE(multisampling, false, "Multisampling (MSAA)", "Anti-aliasing with multisampled attachments resolved at the end of the pass") \
	SAMPLE(instancing, false, "Instancing", "Thousands of rocks in one draw, each with data of its own") \
	SAMPLE(indirectdraw, false, "Indirect drawing", "Draw calls read from a GPU buffer, many meshes and instances each") \
	SAMPLE(computeparticles, false, "Compute particles", "A particle system moved by a compute shader and drawn as points") \
	SAMPLE(descriptorindexing, false, "Bindless textures", "One descriptor array of textures, indexed per object in the shader") \
	SAMPLE(dynamicrendering, false, "Dynamic rendering", "Rendering without render pass and framebuffer objects") \
	SAMPLE(imgui, false, "On-screen UI", "Dear ImGui drawn over a 3D scene, with windows of its own") \
	SAMPLE(meshshader, false, "Mesh shaders", "Geometry made by task and mesh shaders, with no vertex input") \
	SAMPLE(rayquery, false, "Ray queries", "Shadows traced against an acceleration structure from a fragment shader")

#define SAMPLE(id, menu, title, description) \
	namespace sample_##id { VulkanExampleBase *createExample(); }
PS5_SAMPLES
#undef SAMPLE

#define SAMPLE(id, menu, title, description) { #id, title, description, menu, sample_##id::createExample },
const Ps5Sample ps5Samples[] = { PS5_SAMPLES };
#undef SAMPLE

const size_t ps5SampleCount = sizeof(ps5Samples) / sizeof(ps5Samples[0]);
