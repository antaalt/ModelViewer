#version 330

const int SHADOW_CASCADE_COUNT = 3;

in vec3 v_position;
in vec3 v_shadow[SHADOW_CASCADE_COUNT];
in vec3 v_normal;
in vec2 v_uv;
in vec4 v_color;
in float v_clipSpaceDepth;

uniform vec3 u_lightDir;
uniform float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
uniform sampler2D u_colorTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_shadowTexture[SHADOW_CASCADE_COUNT];

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

// Compute TBN matrix.
// TODO compute this offline.
// https://stackoverflow.com/questions/5255806/how-to-calculate-tangent-and-binormal
mat3 computeTBN()
{
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
#if 0
	t = cross( cross( n, t ), t ); // orthonormalization of the tangent vector
	b = cross( b, cross( b, n ) ); // orthonormalization of the binormal vectors to the normal vector
	b = cross( cross( t, b ), t ); // orthonormalization of the binormal vectors to the tangent vector
#else
	// Gran-Schmidt method
	t = t - n * dot( t, n ); // orthonormalization ot the tangent vectors
	b = b - n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector
	b = b - t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
#endif
	return mat3(t, b, n);
}

void main(void)
{
	// Normal mapping
	mat3 tbn = computeTBN();
	float mipmaplevel = mipMapLevel(v_uv);
	vec3 normalMap = textureLod(u_normalTexture, v_uv, mipmaplevel).rgb;
	normalMap = normalMap * 2.0 - 1.0;
	normalMap = normalize(tbn * normalMap);

	// Shadow
	const float bias = 0.005;
	// TODO move out of shader
	const float near = 0.01f;
	const float far = 100.f;
	vec3 visibility = vec3(1);
	float cascadeEndClipSpace[SHADOW_CASCADE_COUNT] = float[](far / 20.f, far / 5.f, far );
	for (int i = 0; i < SHADOW_CASCADE_COUNT; i++)
	{
		if (v_clipSpaceDepth <= u_cascadeEndClipSpace[i])
		{
			if (texture(u_shadowTexture[i], v_shadow[i].xy).x < v_shadow[i].z - bias)
			{
#if 0 // Debug cascades
				visibility = vec3(
					(i == 0) ? 1.0 : 0.0,
					(i == 1) ? 1.0 : 0.0,
					(i == 2) ? 1.0 : 0.0
				);
#else
				visibility *= 0.1;
#endif
			}
			break;
		}
	}

	// Shading
	float cosTheta = clamp(dot(normalMap, normalize(u_lightDir)), 0.0, 1.0);
	vec4 color = v_color * textureLod(u_colorTexture, v_uv, mipmaplevel);
	vec3 indirect = 0.1 * color.rgb;
	vec3 direct = visibility * color.rgb * cosTheta;
	o_color = vec4(indirect + direct, color.a);
}
