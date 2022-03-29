#version 450

#include "color.glsl"
#include "brdf.glsl"

const int SHADOW_CASCADE_COUNT = 3;

layout(location = 0) out vec4 o_color;

layout(location = 0) in vec2 v_uv;

layout(set = 0, binding = 0) uniform sampler2D u_positionTexture;
layout(set = 0, binding = 1) uniform sampler2D u_albedoTexture;
layout(set = 0, binding = 2) uniform sampler2D u_normalTexture;
layout(set = 0, binding = 3) uniform sampler2D u_depthTexture;
layout(set = 0, binding = 4) uniform sampler2D u_materialTexture;

layout(set = 0, binding = 5) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
	mat4 u_viewInverse;
	mat4 u_projectionInverse;
};

layout (set = 1, binding = 0) uniform DirectionalLightUniformBuffer {
	vec3 u_lightDirection;
	float u_lightIntensity;
	vec3 u_lightColor;
	mat4 u_worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
	float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
};

layout(set = 1, binding = 1) uniform sampler2DArray u_shadowMap;

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

vec3 computeDirectionalShadows(vec3 from)
{
	const float bias = 0.0003; // Low value to avoid peter panning
	vec3 visibility = vec3(1);
	float diffusion[3] = float[](1000.f, 3000.f, 5000.f); // TODO parameter relative to frustum size
	for (int iCascade = 0; iCascade < SHADOW_CASCADE_COUNT; iCascade++)
	{
		// Small offset to blend cascades together smoothly
		float offset = random(v_uv.xyy, iCascade) * 0.0001;
		if (texture(u_depthTexture, v_uv).x <= (u_cascadeEndClipSpace[iCascade] + offset))
		{
#if 0 // Debug cascades
			visibility = vec3(
				(iCascade == 0) ? 1.0 : 0.0,
				(iCascade == 1) ? 1.0 : 0.0,
				(iCascade == 2) ? 1.0 : 0.0
			);
#else
			vec4 lightTextureSpace = u_worldToLightTextureSpace[iCascade] * vec4(from, 1.0);
			lightTextureSpace /= lightTextureSpace.w;
#if defined(AKA_FLIP_UV)
			lightTextureSpace.y = 1.0 - lightTextureSpace.y;
#endif
	#if 1 // PCF
			const int pass = 16;
			if (lightTextureSpace.z > -1.0 && lightTextureSpace.z < 1.0)
			{
				for (int iPoisson = 0; iPoisson < 16; iPoisson++)
				{
					int index = int(16.0 * random(v_uv.xyy, iPoisson)) % 16;
					vec2 uv = lightTextureSpace.xy + poissonDisk[index] / diffusion[iCascade];
					if (texture(u_shadowMap, vec3(uv, iCascade)).r < lightTextureSpace.z - bias)
					{
						visibility -= 1.f / pass;
					}
				}
			}
	#else // Linear
			if (lightTextureSpace.z > -1.0 && lightTextureSpace.z < 1.0)
			{
				float dist = texture(u_shadowMap, vec3(lightTextureSpace.xy, iCascade)).r;
				if (lightTextureSpace.w > 0 && dist < lightTextureSpace.z - bias)
				{
					visibility = vec3(0);
				}
			}
	#endif
#endif
			break;
		}
	}
	return visibility;
}


void main(void)
{
	vec3 position = texture(u_positionTexture, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normalTexture, v_uv).rgb;
	vec3 albedo   = sRGB2Linear(texture(u_albedoTexture, v_uv).rgb); // To Linear space
	vec3 material = texture(u_materialTexture, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	vec3 L = normalize(u_lightDirection);
	vec3 N = normalize(normal);
	vec3 V = normalize(vec3(u_viewInverse[3]) - position);
	vec3 I = -V;

	// Shadow
	vec3 visibility = computeDirectionalShadows(position);

	// Shading
	float distance = 1.0; // TODO use physical data (sun distance & intensity)
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = u_lightColor * u_lightIntensity * attenuation;

	vec3 Lo = BRDF(albedo, metalness, roughness, L, V, N) * radiance * visibility;
	//Lo = texture(u_shadowMap, vec3(v_uv, 0)).rgb;
	o_color = vec4(Lo, 1);
}
