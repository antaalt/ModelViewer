#version 330

layout(location = 0) out vec4 o_color;

in vec2 v_uv;

uniform sampler2D u_position;
uniform sampler2D u_albedo;
uniform sampler2D u_normal;
uniform sampler2D u_roughness;
uniform samplerCube u_skybox;

uniform vec3 u_cameraPos;

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return vec3(clamp((x*(a*x+b))/(x*(c*x+d)+e), 0, 1));
}

void main(void)
{
	vec3 position = texture(u_position, v_uv).rgb;
	vec3 normal   = texture(u_normal, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedo, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_roughness, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;


	vec3 N = normalize(normal);
	vec3 V = normalize(u_cameraPos - position);
	vec3 I = -V;

	// Reflection
	vec3 reflection = texture(u_skybox, reflect(I, N)).rgb;

	// Basic ambient shading
	vec3 indirect = 0.03 * albedo; // ao broken for some
	vec3 color = indirect + reflection * 0.01; // TODO use irradiance map

	o_color = vec4(color, 1.0);
}
