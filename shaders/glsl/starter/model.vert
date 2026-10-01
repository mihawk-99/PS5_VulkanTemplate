#version 450
// Starter - the model's vertices in world space, its normals and texture coordinates.
// Copyright (C) 2026 Mihawk - MIT

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightDirection;
	vec4 cameraPosition;
} ubo;

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec2 outUV;

void main()
{
	vec4 world = ubo.model * vec4(inPos, 1.0);
	outWorldPos = world.xyz;
	outNormal = mat3(ubo.model) * inNormal;
	outUV = inUV;
	gl_Position = ubo.projection * ubo.view * world;
}
