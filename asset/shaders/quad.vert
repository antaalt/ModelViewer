#version 450
layout (location = 0) in vec2 a_position;

layout (location = 0) out vec2 v_uv;

void main(void)
{
	v_uv = a_position * 0.5 + 0.5;
#if defined(AKA_FLIP_UV)
	v_uv.y = 1.f - v_uv.y;
#endif
	gl_Position = vec4(a_position, 0.0, 1.0);
}
