static const float PI = 3.14159265359;

cbuffer ModelUniformBuffer : register(b0)
{
	float4x4 u_model;
	float3x3 u_normalMatrix;
	float4 u_color;
};
cbuffer CameraUniformBuffer : register(b1)
{
	float4x4 u_view;
	float4x4 u_projection;
};

cbuffer PointLightUniformBuffer : register(b2)
{
	float3 u_lightPosition;
	float u_lightIntensity;
	float3 u_lightColor;
	float u_farPointLight;
};
cbuffer ViewportUniformBuffer : register(b3)
{
	float2 u_screen;
};

Texture2D    u_positionTexture;
SamplerState u_positionSampler;
Texture2D    u_albedoTexture;
SamplerState u_albedoSampler;
Texture2D    u_normalTexture;
SamplerState u_normalSampler;
Texture2D    u_materialTexture;
SamplerState u_materialSampler;

TextureCube  u_shadowMap;
SamplerState u_shadowMapSampler;

struct vs_in
{
	float3 position : POS;
};

float4 vs_main(vs_in input) : SV_POSITION
{
	return mul(u_projection, mul(u_view, mul(u_model, float4(input.position, 1.0))));
}

static const float3 sampleOffsetDirections[20] =  {
	float3( 1,  1,  1), float3( 1, -1,  1), float3(-1, -1,  1), float3(-1,  1,  1),
	float3( 1,  1, -1), float3( 1, -1, -1), float3(-1, -1, -1), float3(-1,  1, -1),
	float3( 1,  1,  0), float3( 1, -1,  0), float3(-1, -1,  0), float3(-1,  1,  0),
	float3( 1,  0,  1), float3(-1,  0,  1), float3( 1,  0, -1), float3(-1,  0, -1),
	float3( 0,  1,  1), float3( 0, -1,  1), float3( 0, -1, -1), float3( 0,  1, -1)
};

float random(float3 seed, int i)
{
	float4 seed4 = float4(seed.x, seed.y, seed.z, i);
	float dot_product = dot(seed4, float4(12.9898,78.233,45.164,94.673));
	return frac(sin(dot_product) * 43758.5453);
}

float3 computePointShadows(float3 from)
{
	float3 fragToLight = from - u_lightPosition;
	float currentDepth = length(fragToLight);
	float shadow  = 0.0;
	float bias    = 0.15;
	float samples = 20;
	float diskRadius = 0.05;
	for(int i = 0; i < samples; ++i)
	{
		float closestDepth = u_shadowMap.Sample(u_shadowMapSampler, fragToLight + sampleOffsetDirections[i] * diskRadius).r;
		closestDepth *= u_farPointLight;
		if(currentDepth - bias > closestDepth)
			shadow += 1.0;
	}
	shadow /= float(samples);
	return float3(1.0 - shadow, 1.0 - shadow, 1.0 - shadow);
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

float4 ps_main(float4 input : SV_POSITION) : SV_TARGET
{
	float2 texcoord = input.xy / u_screen;
	float4 position = u_positionTexture.Sample(u_positionSampler, texcoord);
	float4 normal = u_normalTexture.Sample(u_normalSampler, texcoord);
	float4 albedo = pow(u_albedoTexture.Sample(u_albedoSampler, texcoord), float4(2.2, 2.2, 2.2, 1.0));
	float4 material = u_materialTexture.Sample(u_materialSampler, texcoord); // AO / roughness / metalness
	float ao = material.r;
	float roughness = material.g;
	float metalness = material.b;

	float3 N = normalize(normal.xyz);
	float3 V = normalize(float3(u_view[0][0], u_view[0][1], u_view[0][2]) - position.xyz);
	float3 I = -V;

	// Shadow
	float3 visibility = computePointShadows(position.xyz);

	// Shading
	float3 L = normalize(u_lightPosition - position);
	float3 H = normalize(V + L);

	float distance = length(u_lightPosition - position);
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
