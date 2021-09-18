#version 450

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv;

layout(binding = 0) uniform sampler2D u_positionTexture;
layout(binding = 1) uniform sampler2D u_albedoTexture;
layout(binding = 2) uniform sampler2D u_normalTexture;
layout(binding = 3) uniform sampler2D u_materialTexture;
layout(binding = 4) uniform samplerCube u_skyboxTexture;

layout(std140, binding = 0) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

void main(void)
{
	vec3 position = texture(u_positionTexture, v_uv).rgb;
	vec3 normal   = texture(u_normalTexture, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedoTexture, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_materialTexture, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;

	vec3 N = normalize(normal);
	vec3 V = normalize(vec3(u_viewInverse[3]) - position);
	vec3 I = -V;

	// Reflection
	vec3 reflection = texture(u_skyboxTexture, reflect(I, N)).rgb;

	// Basic ambient shading
	vec3 indirect = 0.03 * albedo; // ao broken for some
	vec3 color = indirect + reflection * 0.01; // TODO use irradiance map

	o_color = vec4(color, 1.0);
}
