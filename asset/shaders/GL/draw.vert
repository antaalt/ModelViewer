#version 330
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_color;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform mat3 u_normalMatrix;

out vec3 v_position;
out vec3 v_normal;
out vec2 v_uv;
out vec4 v_color;
out vec3 v_forward;

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

	v_forward = (inverse(u_view) * vec4(0, 0, 1, 0)).xyz;
	v_position = gl_Position.xyz;
	v_normal = normalize(u_normalMatrix * a_normal);
	v_uv = a_uv;
	v_color = a_color;
}
