
static const int SHADOW_CASCADE_COUNT = 3;
static const float PI = 3.14159265359;

cbuffer DirectionalLightUniformBuffer: register(b0)
{
	float3 u_lightDirection;
	float u_lightIntensity;
	float3 u_lightColor;
	float4x4 u_worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
	float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
};

cbuffer CameraUniformBuffer: register(b1)
{
	float4x4 u_view;
	float4x4 u_projection;
	float4x4 u_viewInverse;
	float4x4 u_projectionInverse;
};

Texture2D    u_positionTexture;
SamplerState u_positionSampler;
Texture2D    u_albedoTexture;
SamplerState u_albedoSampler;
Texture2D    u_normalTexture;
SamplerState u_normalSampler;
Texture2D    u_materialTexture;
SamplerState u_materialSampler;

Texture2D    u_depthTexture;
SamplerState u_depthSampler;
Texture2D    u_shadowMap[SHADOW_CASCADE_COUNT];
SamplerState u_shadowMapSampler[SHADOW_CASCADE_COUNT];

struct vs_in
{
	float2 position : POS;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float2 texcoord : TEX;
};

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.texcoord = input.position * float2(0.5, 0.5) + float2(0.5, 0.5);
	output.position = float4(input.position, 0.0, 1.0);
	output.texcoord.y = 1.f - output.texcoord.y;
	return output;
}

static const uint sampleCount = 16;
static const float2 poissonDisk[sampleCount] = {
	float2( -0.94201624, -0.39906216 ),
	float2( 0.94558609, -0.76890725 ),
	float2( -0.094184101, -0.92938870 ),
	float2( 0.34495938, 0.29387760 ),
	float2( -0.91588581, 0.45771432 ),
	float2( -0.81544232, -0.87912464 ),
	float2( -0.38277543, 0.27676845 ),
	float2( 0.97484398, 0.75648379 ),
	float2( 0.44323325, -0.97511554 ),
	float2( 0.53742981, -0.47373420 ),
	float2( -0.26496911, -0.41893023 ),
	float2( 0.79197514, 0.19090188 ),
	float2( -0.24188840, 0.99706507 ),
	float2( -0.81409955, 0.91437590 ),
	float2( 0.19984126, 0.78641367 ),
	float2( 0.14383161, -0.14100790 )
};

float random(float3 seed, int i)
{
	float4 seed4 = float4(seed.x, seed.y, seed.z, i);
	float dot_product = dot(seed4, float4(12.9898,78.233,45.164,94.673));
	return frac(sin(dot_product) * 43758.5453);
}

float3 computeDirectionalShadows(float3 from, float2 uv)
{
	const float bias = 0.0003; // Low value to avoid peter panning
	float3 visibility = float3(1,1,1);
	float diffusion[3] = { 1000.f, 3000.f, 5000.f}; // TODO parameter
	for (int iCascade = 0; iCascade < SHADOW_CASCADE_COUNT; iCascade++)
	{
		// Small offset to blend cascades together smoothly
		float offset = random(uv.xyy, iCascade) * 0.0001;
		if (u_depthTexture.Sample(u_depthSampler, uv).x <= (u_cascadeEndClipSpace[iCascade] + offset))
		{
#if 0 // Debug cascades
			visibility = float3(
				(iCascade == 0) ? 1.0 : 0.0,
				(iCascade == 1) ? 1.0 : 0.0,
				(iCascade == 2) ? 1.0 : 0.0
			);
#else
			// PCF
			float4 lightTextureSpace = mul(u_worldToLightTextureSpace[iCascade], float4(from.x, from.y, from.z, 1.0));
			lightTextureSpace /= lightTextureSpace.w;
			for (int iPoisson = 0; iPoisson < sampleCount; iPoisson++)
			{
				int index = int(sampleCount * random(uv.xyy, iPoisson)) % sampleCount;
				float2 uv = lightTextureSpace.xy + poissonDisk[index] / diffusion[iCascade];
				if (u_shadowMap[iCascade].Sample(u_shadowMapSampler[iCascade], uv).z < lightTextureSpace.z - bias)
				{
					visibility -= 1.f / sampleCount;
				}
			}
#endif
			break;
		}
	}
	return visibility;
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

float DistributionGGX(float3 N, float3 H, float roughness)
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
float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	float ggx2  = GeometrySchlickGGX(NdotV, roughness);
	float ggx1  = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	float4 position = u_positionTexture.Sample(u_positionSampler, input.texcoord);
	float4 normal = u_normalTexture.Sample(u_normalSampler, input.texcoord);
	float4 albedo = pow(u_albedoTexture.Sample(u_albedoSampler, input.texcoord), float4(2.2, 2.2, 2.2, 1.0));
	float4 material = u_materialTexture.Sample(u_materialSampler, input.texcoord); // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	float3 N = normalize(normal.xyz);
	float3 V = normalize(float3(u_viewInverse[0][0], u_viewInverse[0][1], u_viewInverse[0][2]) - position.xyz);
	float3 I = -V;

	// Shadow
	float3 visibility = computeDirectionalShadows(position.xyz, input.texcoord);

	// Shading
	float3 L = normalize(u_lightDirection);
	float3 H = normalize(V + L);

	float distance = 1.0;
	float attenuation = 1.0 / (distance * distance);
	float3 radiance = u_lightColor * u_lightIntensity * attenuation;

	// non metallic surface always 0.04
	float3 F0 = float3(0.04, 0.04, 0.04);
	F0        = lerp(F0, albedo.xyz, float3(metalness, metalness, metalness));
	float3 F  = fresnelSchlick(max(dot(H, V), 0.0), F0);

	float NDF = DistributionGGX(N, H, roughness);
	float G   = GeometrySmith(N, V, L, roughness);

	float3 numerator    = NDF * G * F;
	float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
	float3 specular     = numerator / max(denominator, 0.001);

	float3 kS = F;
	float3 kD = float3(1.0, 1.0, 1.0) - kS;
	kD *= 1.0 - metalness;

	float NdotL = max(dot(N, L), 0.0);
	float3 Lo = (kD * albedo / PI + specular) * radiance * NdotL * visibility;

	return float4(Lo.x, Lo.y, Lo.z, 1);
}
