#version 330
layout (location = 0) in vec3 a_position;

uniform mat4 u_mvp;

void main(void)
{
	gl_Position = u_mvp * vec4(a_position, 1.0);
}