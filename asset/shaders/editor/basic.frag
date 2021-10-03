#version 450

layout(location = 0) in vec3 v_normal;
layout(location = 1) in vec3 v_forward;
layout(location = 2) in vec3 v_color;

layout(location = 0) out vec4 o_color;

void main()
{
	float costheta = dot(v_forward, v_normal);
	o_color = vec4(v_color * clamp(costheta, 0.1, 1.0), 1.0);
}
