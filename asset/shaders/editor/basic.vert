#version 450

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_uv;
layout(location = 3) in vec4 a_color;

layout(location = 0) out vec3 v_normal;
layout(location = 1) out vec3 v_forward;
layout(location = 2) out vec3 v_color;

layout(binding = 0, std140) uniform CameraUniformBuffer
{
	mat4 u_mvp;
};

void main()
{
	gl_Position = u_mvp * vec4(a_position, 1.0);
	v_normal = a_normal;
	v_forward = mat3(u_mvp) * vec3(0, 0, -1);
	v_color = a_color.xyz;
}
