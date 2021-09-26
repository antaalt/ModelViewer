#version 450 core

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv;

layout(binding = 0) uniform sampler2D u_texture;

void main()
{
	vec4 albedo = texture(u_texture, v_uv);
	if (bool(albedo.a < 0.8)) { // TODO use threshold
		discard;
	}
	o_color = albedo;
}
