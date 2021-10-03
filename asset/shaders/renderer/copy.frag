#version 450

layout(location = 0) in vec2 v_uv;

layout(location = 0) out vec4 o_color;

layout(binding = 0) uniform sampler2D u_input;

void main(void)
{
	o_color = texture(u_input, v_uv);
}
