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

vec2 poissonDisk[16] = vec2[](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 ),
	vec2( -0.91588581, 0.45771432 ),
	vec2( -0.81544232, -0.87912464 ),
	vec2( -0.38277543, 0.27676845 ),
	vec2( 0.97484398, 0.75648379 ),
	vec2( 0.44323325, -0.97511554 ),
	vec2( 0.53742981, -0.47373420 ),
	vec2( -0.26496911, -0.41893023 ),
	vec2( 0.79197514, 0.19090188 ),
	vec2( -0.24188840, 0.99706507 ),
	vec2( -0.81409955, 0.91437590 ),
	vec2( 0.19984126, 0.78641367 ),
	vec2( 0.14383161, -0.14100790 )
);

float random(vec3 seed, int i)
{
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
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
	vec3 normalMap = texture(u_normalTexture, v_uv).rgb;
	normalMap = normalMap * 2.0 - 1.0;
	normalMap = normalize(tbn * normalMap);

	// Shadow
	const float bias = 0.0005; // Low value to avoid peter panning
	vec3 visibility = vec3(1);
	float diffusion[3] = float[](1000.f, 3000.f, 5000.f);
	for (int iCascade = 0; iCascade < SHADOW_CASCADE_COUNT; iCascade++)
	{
		if (v_clipSpaceDepth <= u_cascadeEndClipSpace[iCascade])
		{
#if 0 // Debug cascades
			visibility = vec3(
				(iCascade == 0) ? 1.0 : 0.0,
				(iCascade == 1) ? 1.0 : 0.0,
				(iCascade == 2) ? 1.0 : 0.0
			);
#else
			// PCF
			const int pass = 16;
			for (int iPoisson = 0; iPoisson < 16; iPoisson++)
			{
				int index = int(16.0 * random(gl_FragCoord.xyy, iPoisson)) % 16;
				vec2 uv = v_shadow[iCascade].xy + poissonDisk[index] / diffusion[iCascade];
				if (texture(u_shadowTexture[iCascade], uv).z < v_shadow[iCascade].z - bias)
				{
					visibility -= 1.f / pass;
				}
			}
#endif
			break;
		}
	}

	// Shading
	float cosTheta = clamp(dot(normalMap, normalize(u_lightDir)), 0.0, 1.0);
	vec4 color = v_color * texture(u_colorTexture, v_uv);
	vec3 indirect = 0.1 * color.rgb;
	vec3 direct = visibility * color.rgb * cosTheta;
	o_color = vec4(indirect + direct, color.a);
}
