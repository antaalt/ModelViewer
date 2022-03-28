#version 450 core

layout(location = 0) in vec3 a_position;

layout(set = 0, binding = 0) uniform LightModelUniformBuffer {
	mat4 u_model;
	mat3 u_normal;
};
layout(set = 1, binding = 0) uniform DirectionalLightUniformBuffer {
	mat4 u_light;
};

void main() {
	gl_Position = u_light * u_model * vec4(a_position, 1.0);
}
