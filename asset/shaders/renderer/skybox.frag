#version 450 core

layout (location = 0) out vec4 o_color;

layout (location = 0) in vec3 v_uv;

layout (binding = 1) uniform samplerCube u_skyboxTexture;

void main()
{
	o_color = texture(u_skyboxTexture, normalize(v_uv));
}
