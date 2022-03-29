#version 450 core

layout(location = 0) in vec4 v_position;

layout(set = 1, binding = 0) uniform PointLightUniformBuffer {
	mat4 u_light;
	vec3 u_lightPos;
	float u_far;
};

void main()
{
	// get distance between fragment and light source
	float lightDistance = length(v_position.xyz - u_lightPos);

	// map to [0;1] range by dividing by far_plane
	lightDistance = lightDistance / u_far;

	// write this as modified depth
	gl_FragDepth = lightDistance;
}
