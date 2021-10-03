#version 450 core

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv;
layout(location = 1) in vec4 v_color;

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
	vec4 albedo = v_color * texture(u_texture, v_uv);
	if (bool(albedo.a < 0.8)) { // TODO use threshold
		discard;
	}
	o_color = albedo;
}
