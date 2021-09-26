#version 450
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 v_uv;
layout (location = 1) out vec4 v_color;

layout(std140, binding = 0) uniform ModelUniformBuffer {
	mat4 u_model;
	mat3 u_normalMatrix;
	vec4 u_color;
};

layout(std140, binding = 1) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

void main(void)
{
	v_uv = a_uv;
#if defined(AKA_FLIP_UV)
	v_uv.y = 1.f - v_uv.y;
#endif
	v_color = u_color;
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 0.0, 1.0);
}
