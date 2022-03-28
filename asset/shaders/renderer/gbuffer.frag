#version 450

layout (location = 0) out vec3 o_position;
layout (location = 1) out vec4 o_albedo;
layout (location = 2) out vec3 o_normal;
layout (location = 3) out vec3 o_roughness;

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec2 v_uv;
layout (location = 3) in vec4 v_color;

layout (set = 1, binding = 1) uniform sampler2D u_colorTexture;
layout (set = 1, binding = 2) uniform sampler2D u_normalTexture;
layout (set = 1, binding = 3) uniform sampler2D u_materialTexture;

void main(void)
{
	// --- Generate albedo
	vec4 albedo = v_color * texture(u_colorTexture, v_uv);

	// --- Generate normals
	// Compute TBN matrix.
	// TODO compute this offline.
	// https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
	// derivations of the fragment position
	vec3 p_dx = dFdx(v_position);
	vec3 p_dy = dFdy(v_position);
	// derivations of the texture coordinate
	vec2 t_dx = dFdx(v_uv);
	vec2 t_dy = dFdy(v_uv);
	// tangent vector and binormal vector
	vec3 n = normalize(v_normal);
	vec3 t = normalize(t_dy.y * p_dx - t_dx.y * p_dy);
	vec3 b = normalize(t_dx.x * p_dy - t_dy.x * p_dx);
	// Gran-Schmidt method
	t = t - n * dot( t, n ); // orthonormalization ot the tangent vectors
	b = b - n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector
	b = b - t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
	mat3 tbn = mat3(t, b, n);
	vec3 normal = texture(u_normalTexture, v_uv).rgb;
	normal = normal * 2.0 - 1.0;
	normal = normalize(tbn * normal);

	// --- Generate depth
	//gl_FragDepth = gl_FragCoord.z;

	// --- Alpha
	if (bool(albedo.a < 0.8)) { // TODO use threshold
		discard;
	}

	o_position = v_position;
	o_normal = normal; // TODO store normal in 0 1 to support unsigned format
	o_albedo = albedo;
	o_roughness = texture(u_materialTexture, v_uv).rgb; // RHM
}
