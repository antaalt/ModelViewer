#version 330
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat4 u_light;
uniform mat3 u_normalMatrix;
uniform vec4 u_color;
uniform vec3 u_lightDir;

out vec3 v_position; // world space
out vec3 v_shadow; // light texture space
out vec3 v_normal; // world space
out vec2 v_uv; // texture space
out vec4 v_color; // color space

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

	v_position = (u_model * vec4(a_position, 1.0)).xyz;
	v_shadow = (u_light * u_model * vec4(a_position, 1.0)).xyz;
	v_normal = normalize(u_normalMatrix * a_normal);
	v_uv = a_uv;
	v_color = u_color * a_color;
}
