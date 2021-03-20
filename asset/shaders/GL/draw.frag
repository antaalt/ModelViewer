#version 330

in vec3 v_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;
in vec3 v_forward;

out vec4 o_color;

void main(void) {
	float cosTheta = max(0.2, dot(v_normal, v_forward));
	o_color = vec4(v_color.rgb * cosTheta, v_color.a);
}
