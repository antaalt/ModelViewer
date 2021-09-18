#version 450
layout (location = 0) in vec3 a_position;

layout(std140, binding = 0) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

layout(std140, binding = 1) uniform ModelUniformBuffer {
	mat4 u_model;
	mat3 u_normalMatrix;
	vec4 u_color;
};

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
