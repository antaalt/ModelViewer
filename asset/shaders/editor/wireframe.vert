#version 450

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec2 a_uv;
layout (location = 3) in vec4 a_color;

layout(binding = 0, std140) uniform ModelUniformBuffer {
	mat4 u_mvp;
};

layout (location = 0) out vec4 v_color;

void main(void) {
	gl_Position = u_mvp * vec4(a_position, 1.0);
	v_color = a_color;
}
