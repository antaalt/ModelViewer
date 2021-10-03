#version 450 core

layout(location = 0) in vec3 a_position;

layout(std140, binding = 0) uniform LightModelUniformBuffer {
	mat4 u_model;
};
layout(std140, binding = 1) uniform DirectionalLightUniformBuffer {
	mat4 u_light;
};

void main() {
	gl_Position = u_light * u_model * vec4(a_position, 1.0);
}
