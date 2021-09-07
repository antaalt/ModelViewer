#version 330 core

layout(location = 0) in vec3 a_position;

layout(std140) uniform LightModelUniformBuffer {
	mat4 u_model;
};

void main()
{
	gl_Position = u_model * vec4(a_position, 1.0);
}
