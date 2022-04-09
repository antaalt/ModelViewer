#version 450
layout (location = 0) in vec3 a_position;

layout(set = 0, binding = 5) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

layout(set = 1, binding = 0) uniform PointLightUniformBuffer {
	mat4 u_model;
	vec3 u_lightPosition;
	float u_lightIntensity;
	vec3 u_lightColor;
	float u_farPointLight;
};

void main(void)
{
	gl_Position = u_projection * u_view * u_model * vec4(a_position, 1.0);
}
