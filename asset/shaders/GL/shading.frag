#version 330

layout(location = 0) out vec4 o_color;

const int SHADOW_CASCADE_COUNT = 3;

in vec2 v_uv;

uniform sampler2D u_position;
uniform sampler2D u_albedo;
uniform sampler2D u_normal;
uniform sampler2D u_depth;

uniform vec3 u_lightDir;
uniform mat4 u_worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
uniform float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
uniform sampler2D u_shadowTexture[SHADOW_CASCADE_COUNT];

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

void main(void)
{
	vec3 position = texture(u_position, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normal, v_uv).rgb;
	vec4 albedo   = texture(u_albedo, v_uv);

	// Shadow
	const float bias = 0.0005; // Low value to avoid peter panning
	vec3 visibility = vec3(1);
	float diffusion[3] = float[](1000.f, 3000.f, 5000.f); // TODO parameter
	for (int iCascade = 0; iCascade < SHADOW_CASCADE_COUNT; iCascade++)
	{
		if (texture(u_depth, v_uv).x <= u_cascadeEndClipSpace[iCascade])
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
			vec4 lightTextureSpace = u_worldToLightTextureSpace[iCascade] * vec4(position, 1.0);
			lightTextureSpace /= lightTextureSpace.w;
			for (int iPoisson = 0; iPoisson < 16; iPoisson++)
			{
				int index = int(16.0 * random(v_uv.xyy, iPoisson)) % 16;
				vec2 uv = lightTextureSpace.xy + poissonDisk[index] / diffusion[iCascade];
				if (texture(u_shadowTexture[iCascade], uv).z < lightTextureSpace.z - bias)
				{
					visibility -= 1.f / pass;
				}
			}
#endif
			break;
		}
	}

	// Shading
	float cosTheta = clamp(dot(normal, normalize(u_lightDir)), 0.0, 1.0);
	vec3 indirect = 0.1 * albedo.rgb;
	vec3 direct = visibility * albedo.rgb * cosTheta;
	o_color = vec4(indirect + direct, albedo.a);
}
