#version 450

#include "brdf.glsl"

layout(location = 0) out vec4 o_color;

//#define QUAD
#ifdef QUAD // When using quad.vert
layout(location = 0) in vec2 v_position;
layout(location = 1) in vec2 v_uv;
#else // When using point.vert
#define v_uv (gl_FragCoord.xy / u_screen)
#endif

layout(binding = 0) uniform sampler2D u_positionTexture;
layout(binding = 1) uniform sampler2D u_albedoTexture;
layout(binding = 2) uniform sampler2D u_normalTexture;
layout(binding = 3) uniform sampler2D u_materialTexture;

layout(binding = 4) uniform samplerCube u_shadowMap;

layout(std140, binding = 0) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

layout(std140, binding = 2) uniform PointLightUniformBuffer {
	vec3 u_lightPosition;
	float u_lightIntensity;
	vec3 u_lightColor;
	float u_farPointLight;
};

#ifndef QUAD
layout(std140, binding = 3) uniform ViewportUniformBuffer {
	vec2 u_screen;
};
#endif

vec3 sampleOffsetDirections[20] = vec3[]
(
	vec3( 1,  1,  1), vec3( 1, -1,  1), vec3(-1, -1,  1), vec3(-1,  1,  1),
	vec3( 1,  1, -1), vec3( 1, -1, -1), vec3(-1, -1, -1), vec3(-1,  1, -1),
	vec3( 1,  1,  0), vec3( 1, -1,  0), vec3(-1, -1,  0), vec3(-1,  1,  0),
	vec3( 1,  0,  1), vec3(-1,  0,  1), vec3( 1,  0, -1), vec3(-1,  0, -1),
	vec3( 0,  1,  1), vec3( 0, -1,  1), vec3( 0, -1, -1), vec3( 0,  1, -1)
);

float random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

vec3 computePointShadows(vec3 from)
{
	vec3 fragToLight = from - u_lightPosition;
	float currentDepth = length(fragToLight);
	float shadow  = 0.0;
	float bias    = 0.15;
	float samples = 20;
	float diskRadius = 0.05;
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = texture(u_shadowMap, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= u_farPointLight;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);
	return vec3(1.0 - shadow);
}

void main(void)
{
	vec3 position = texture(u_positionTexture, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normalTexture, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedoTexture, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_materialTexture, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	vec3 L = normalize(u_lightPosition - position);
	vec3 N = normalize(normal);
	vec3 V = normalize(vec3(u_viewInverse[3]) - position);
	vec3 I = -V;

	// Shadow
	vec3 visibility = computePointShadows(position);

 	// Shading
	float distance    = length(u_lightPosition - position);
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance     = u_lightColor * u_lightIntensity * attenuation;

	vec3 Lo = BRDF(albedo, metalness, roughness, L, V, N) * radiance * visibility;
	o_color = vec4(Lo, 1);
}
