#version 330
layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_color;

layout(std140) uniform ModelUniformBuffer {
	mat4 u_model;
	mat3 u_normalMatrix;
	vec4 u_color;
};
layout(std140) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

out vec3 v_position; // world space
out vec3 v_normal; // world space
out vec2 v_uv; // texture space
out vec4 v_color;

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);

	v_position = vec3(u_model * vec4(a_position, 1.0));
	v_normal = normalize(u_normalMatrix * a_normal);
	v_uv = a_uv;
	v_color = u_color * a_color;
}
