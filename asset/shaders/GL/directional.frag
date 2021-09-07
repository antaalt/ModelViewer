#version 330

layout(location = 0) out vec4 o_color;

const int SHADOW_CASCADE_COUNT = 3;
const float PI = 3.14159265359;

in vec2 v_uv;

uniform sampler2D u_positionTexture;
uniform sampler2D u_albedoTexture;
uniform sampler2D u_normalTexture;
uniform sampler2D u_depthTexture;
uniform sampler2D u_materialTexture;

uniform sampler2D u_shadowMap[SHADOW_CASCADE_COUNT];

layout (std140) uniform DirectionalLightUniformBuffer {
	vec3 u_lightDirection;
	float u_lightIntensity;
	vec3 u_lightColor;
	mat4 u_worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
	float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
};

layout(std140) uniform CameraUniformBuffer {
	mat4 u_view;
	mat4 u_projection;
};

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
	float diffusion[3] = float[](1000.f, 3000.f, 5000.f); // TODO parameter
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
			// PCF
			const int pass = 16;
			vec4 lightTextureSpace = u_worldToLightTextureSpace[iCascade] * vec4(from, 1.0);
			lightTextureSpace /= lightTextureSpace.w;
			for (int iPoisson = 0; iPoisson < 16; iPoisson++)
			{
				int index = int(16.0 * random(v_uv.xyy, iPoisson)) % 16;
				vec2 uv = lightTextureSpace.xy + poissonDisk[index] / diffusion[iCascade];
				if (texture(u_shadowMap[iCascade], uv).z < lightTextureSpace.z - bias)
				{
					visibility -= 1.f / pass;
				}
			}
#endif
			break;
		}
	}
	return visibility;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a      = roughness*roughness;
	float a2     = a*a;
	float NdotH  = max(dot(N, H), 0.0);
	float NdotH2 = NdotH*NdotH;

	float num   = a2;
	float denom = (NdotH2 * (a2 - 1.0) + 1.0);
	denom = PI * denom * denom;

	return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = (roughness + 1.0);
	float k = (r*r) / 8.0;

	float num   = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = GeometrySchlickGGX(NdotV, roughness);
	float ggx1  = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

void main(void)
{
	vec3 position = texture(u_positionTexture, v_uv).rgb; // TODO get position from depth buffer ? to save memory
	vec3 normal   = texture(u_normalTexture, v_uv).rgb;
	vec3 albedo   = pow(texture(u_albedoTexture, v_uv).rgb, vec3(2.2)); // To Linear space
	vec3 material = texture(u_materialTexture, v_uv).rgb; // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	vec3 N = normalize(normal);
	vec3 V = normalize(vec3(u_view[3]) - position);
	vec3 I = -V;

	// Shadow
	vec3 visibility = computeDirectionalShadows(position);

	// Shading
	vec3 L = normalize(u_lightDirection);
	vec3 H = normalize(V + L);

	float distance = 1.0;
	float attenuation = 1.0 / (distance * distance);
	vec3 radiance = u_lightColor * u_lightIntensity * attenuation;

	// non metallic surface always 0.04
	vec3 F0 = vec3(0.04);
	F0      = mix(F0, albedo, vec3(metalness));
	vec3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);

	vec3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	vec3 specular     = numerator / max(denominator, 0.001);

	vec3 kS = F;
	vec3 kD = vec3(1.0) - kS;
	kD *= 1.0 - metalness;

	float NdotL = max(dot(N, L), 0.0);
	vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * visibility;

	o_color = vec4(Lo, 1);
}
