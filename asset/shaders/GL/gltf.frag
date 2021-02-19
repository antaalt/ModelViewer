#version 330

in vec3 v_position;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;
in mat3 v_normalMatrix;

uniform sampler2D u_colorTexture;
uniform sampler2D u_normalTexture;

out vec4 o_color;

void main(void) {
	// Compute TBN matrix.
	// TODO compute this offline.
	vec3 n = normalize(v_normalMatrix * v_normal);
	// derivations of the fragment position
	vec3 pos_dx = dFdx(v_position);
	vec3 pos_dy = dFdy(v_position);
	// derivations of the texture coordinate
	vec2 texC_dx = dFdx(v_uv);
	vec2 texC_dy = dFdy(v_uv);
	// tangent vector and binormal vector
	vec3 t = texC_dy.y * pos_dx - texC_dx.y * pos_dy;
	vec3 b = texC_dx.x * pos_dy - texC_dy.x * pos_dx;
	// Gran-Schmidt method
	t = t - n * dot( t, n ); // orthonormalization ot the tangent vectors
	b = b - n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector
	b = b - t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
	mat3 TBN = mat3(t, b, n);

	vec3 normalMap = texture(u_normalTexture, v_uv).rgb;
	normalMap = normalMap * 2.0 - 1.0;
	normalMap = normalize(TBN * normalMap);


	float cosTheta = clamp(dot(normalMap, normalize(vec3(1,1,-1))), 0.2, 1);
	vec4 color = v_color * texture(u_colorTexture, v_uv);
	o_color = vec4(color.rgb * cosTheta, color.a);
}
