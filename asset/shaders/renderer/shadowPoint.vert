#version 450 core

layout(location = 0) in vec3 a_position;
layout(location = 0) out vec4 v_position;

layout(set = 0, binding = 1) uniform LightModelUniformBuffer {
	mat4 u_model;
	mat3 u_normal;
};

layout(set = 1, binding = 0) uniform PointLightUniformBuffer {
	mat4 u_light;
	vec3 u_lightPos;
	float u_far;
};

void main()
{
	v_position = u_model * vec4(a_position, 1.0);
	gl_Position = u_light * v_position;
}
