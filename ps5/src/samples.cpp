/*
 * PS5 Vulkan Template - the samples linked into the title.
 *
 * One line a sample: SAMPLE(id, in the menu, title, description). With one
 * line only, the title is that program: it starts with no menu, and ends when
 * the program does (ps5/tools/new-title.py makes such titles). The build
 * (ps5/CMakeLists.txt) compiles examples/<id>/<id>.cpp for every line here, in
 * a namespace of its own, and packages shaders/glsl/<id>/. A sample goes in the
 * menu only once it has been proven on the console (ps5/README.md); until then
 * test runs reach it and the menu does not.
 *
 * A sample may have variants for test runs: VARIANTS(id, "a b c") lets a run
 * name "id.a", which starts the sample with ps5.variant "a" (the UI gallery
 * starts on that design). A run of "all" or "menu" runs a sample's variants
 * instead of the sample itself.
 *
 * Copyright (C) 2026 Mihawk
 *
 * This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
 */
#include "ps5_samples.h"

#define PS5_SAMPLES                                                                                          \
	SAMPLE(starter, true, "Starter", "The program a new homebrew grows from: a textured, lit glTF model and a settings window") \
	SAMPLE(gltfloading, true, "glTF model loading", "A glTF scene: meshes, materials, textures, node hierarchy") \
	SAMPLE(texturemipmapgen, true, "Mipmaps made at run time", "A texture's mip chain made with image blits, and how filtering uses it") \
	SAMPLE(pbrtexture, true, "Physically based rendering", "Metal and roughness maps lit by an HDR environment (image based lighting)") \
	SAMPLE(shadowmapping, true, "Shadow mapping", "A directional light's depth map, filtered with PCF") \
	SAMPLE(deferred, true, "Deferred shading", "A G-buffer in several render targets, then many lights in one pass") \
	SAMPLE(bloom, true, "Bloom", "Bright parts blurred in two separable passes and added back") \
	SAMPLE(multisampling, true, "Multisampling (MSAA)", "Anti-aliasing with multisampled attachments resolved at the end of the pass") \
	SAMPLE(instancing, true, "Instancing", "Thousands of rocks in one draw, each with data of its own") \
	SAMPLE(indirectdraw, true, "Indirect drawing", "Draw calls read from a GPU buffer, many meshes and instances each") \
	SAMPLE(computeparticles, true, "Compute particles", "A particle system moved by a compute shader and drawn as points") \
	SAMPLE(descriptorindexing, true, "Bindless textures", "One descriptor array of textures, indexed per object in the shader") \
	SAMPLE(dynamicrendering, true, "Dynamic rendering", "Rendering without render pass and framebuffer objects") \
	SAMPLE(imgui, true, "On-screen UI", "Dear ImGui drawn over a 3D scene, with windows of its own") \
	SAMPLE(meshshader, true, "Mesh shaders", "Geometry made by task and mesh shaders, with no vertex input") \
	SAMPLE(rayquery, true, "Ray queries", "Shadows traced against an acceleration structure from a fragment shader")

#define SAMPLE(id, menu, title, description) \
	namespace sample_##id { VulkanExampleBase *createExample(); }
PS5_SAMPLES
#undef SAMPLE

#define SAMPLE(id, menu, title, description) { #id, title, description, menu, sample_##id::createExample },
const Ps5Sample ps5Samples[] = { PS5_SAMPLES };
#undef SAMPLE

const size_t ps5SampleCount = sizeof(ps5Samples) / sizeof(ps5Samples[0]);

#define PS5_VARIANTS

#define VARIANTS(id, names) { #id, names },
const Ps5Variants ps5Variants[] = { PS5_VARIANTS { nullptr, nullptr } };
#undef VARIANTS

const size_t ps5VariantCount = sizeof(ps5Variants) / sizeof(ps5Variants[0]) - 1;
