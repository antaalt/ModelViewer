#version 330

in vec3 v_position;
in vec3 v_shadow;
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;
in mat3 v_normalMatrix;

uniform vec3 u_lightDir;

uniform sampler2D u_colorTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_shadowTexture;

out vec4 o_color;

// Does not take into account GL_TEXTURE_MIN_LOD/GL_TEXTURE_MAX_LOD/GL_TEXTURE_LOD_BIAS,
// nor implementation-specific flexibility allowed by OpenGL spec
float mipMapLevel(in vec2 texture_coordinate) // in texel units
{
	vec2  dx_vtc        = dFdx(texture_coordinate);
	vec2  dy_vtc        = dFdy(texture_coordinate);
	float delta_max_sqr = max(dot(dx_vtc, dx_vtc), dot(dy_vtc, dy_vtc));
	float mml = 0.5 * log2(delta_max_sqr);
	return max( 0, mml ); // Thanks @Nims
}

void main(void) {
	// Compute TBN matrix.
	// TODO compute this offline.
	// https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
	vec3 n = normalize(v_normalMatrix * v_normal);
	// derivations of the fragment position
	vec3 pos_dx = dFdx(v_position);
	vec3 pos_dy = dFdy(v_position);
	// derivations of the texture coordinate
	vec2 texC_dx = dFdx(v_uv);
	vec2 texC_dy = dFdy(v_uv);
	// tangent vector and binormal vector
	vec3 t = normalize(texC_dy.y * pos_dx - texC_dx.y * pos_dy);
	vec3 b = normalize(texC_dx.x * pos_dy - texC_dy.x * pos_dx);
#if 1
	t = cross( cross( n, t ), t ); // orthonormalization of the tangent vector
	b = cross( b, cross( b, n ) ); // orthonormalization of the binormal vectors to the normal vector
	b = cross( cross( t, b ), t ); // orthonormalization of the binormal vectors to the tangent vector
#else
	// Gran-Schmidt method
	t = t - n * dot( t, n ); // orthonormalization ot the tangent vectors
	b = b - n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector
	b = b - t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
#endif
	mat3 TBN = mat3(t, b, n);

	float mipmaplevel = mipMapLevel(v_uv);

	vec3 normalMap = textureLod(u_normalTexture, v_uv, mipmaplevel).rgb;
	normalMap = normalMap * 2.0 - 1.0;
	normalMap = normalize(TBN * normalMap);

	float bias = 0.005;
	float visibility = 1.0;
	if (texture(u_shadowTexture, v_shadow.xy).z < v_shadow.z - bias)
	{
		visibility = 0.1;
	}

	float cosTheta = clamp(dot(normalMap, normalize(u_lightDir)), 0.2, 1);
	vec4 color = v_color * textureLod(u_colorTexture, v_uv, mipmaplevel);
	o_color = vec4(visibility, visibility, visibility, 1);
	o_color = vec4(visibility * color.rgb * cosTheta, color.a);
}
