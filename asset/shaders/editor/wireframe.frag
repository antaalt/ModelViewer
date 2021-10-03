#version 450

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec4 v_color;

layout (location = 0) out vec4 o_color;

void main(void)
{
	o_color = v_color;
}
