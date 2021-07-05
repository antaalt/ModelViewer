#version 330
layout (location = 0) in vec2 a_position;

out vec2 v_uv;

void main(void)
{
    v_uv = a_position * vec2(0.5) + vec2(0.5);
    gl_Position = vec4(a_position, 0.0, 1.0);
}
