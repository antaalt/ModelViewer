#version 450
layout (location = 0) in vec2 a_position;
layout (location = 1) in vec2 a_uv;

layout (location = 0) out vec2 v_uv;

void main(void)
{
	v_uv = a_uv;
	gl_Position = vec4(a_position, 0.0, 1.0);
}