#version 330
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_color;

const int SHADOW_CASCADE_COUNT = 3;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_light[SHADOW_CASCADE_COUNT];
uniform mat3 u_normalMatrix;
uniform vec4 u_color;
uniform vec3 u_lightDir;

out vec3 v_position; // world space
out vec3 v_shadow[SHADOW_CASCADE_COUNT]; // light texture space
out vec3 v_normal; // world space
out vec2 v_uv; // texture space
out vec4 v_color; // color space

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

	v_position = vec3(u_model * vec4(a_position, 1.0));
	for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
	{
		// Compute shadow texture coordinate
		vec4 lightTextureSpace = u_light[i] * vec4(v_position, 1.0);
		v_shadow[i] = lightTextureSpace.xyz;// / lightTextureSpace.w;
	}
	v_normal = normalize(u_normalMatrix * a_normal);
	v_uv = a_uv;
	v_color = u_color * a_color;
}
