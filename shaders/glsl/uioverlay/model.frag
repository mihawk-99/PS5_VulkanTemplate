#version 450
// Starter - the model's base colour texture, a warm key light, a cool fill from
// below and a soft highlight. Lighting is done in linear space; the texture and
// the swapchain are UNORM holding sRGB values, so the colour is decoded on the way
// in and encoded on the way out.
// Copyright (C) 2026 Mihawk - MIT

layout (set = 0, binding = 0) uniform UBO
{
	mat4 projection;
	mat4 view;
	mat4 model;
	vec4 lightDirection;
	vec4 cameraPosition;
} ubo;
layout (set = 1, binding = 0) uniform sampler2D baseColor;

layout (location = 0) in vec3 inWorldPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
	vec3 n = normalize(inNormal);
	vec3 v = normalize(ubo.cameraPosition.xyz - inWorldPos);
	if (dot(n, v) < 0.0) {
		n = -n; // both sides of thin surfaces are lit
	}
	vec3 l = normalize(-ubo.lightDirection.xyz);
	vec3 albedo = pow(texture(baseColor, inUV).rgb, vec3(2.2));
	float key = max(dot(n, l), 0.0);
	// The world's Y is down: the fill comes from +Y
	float fill = max(dot(n, vec3(0.0, 1.0, 0.0)), 0.0) * 0.25 + 0.15;
	float highlight = pow(max(dot(n, normalize(l + v)), 0.0), 48.0) * 0.35 * key;
	vec3 linearColor = albedo * (key * vec3(1.0, 0.92, 0.80) + fill * vec3(0.55, 0.65, 0.85)) + highlight;
	outFragColor = vec4(pow(clamp(linearColor, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
