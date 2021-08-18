#version 330 core

out vec4 o_color;

in vec3 v_uv;

uniform samplerCube u_skyboxTexture;

void main()
{
    o_color = texture(u_skyboxTexture, v_uv);
}
