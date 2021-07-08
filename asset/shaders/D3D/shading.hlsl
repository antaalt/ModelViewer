
static const int SHADOW_CASCADE_COUNT = 3;

cbuffer constants : register(b0)
{
	float3 u_cameraPos;
	float3 u_lightDir;
	float4x4 u_worldToLightTextureSpace[SHADOW_CASCADE_COUNT];
	float u_cascadeEndClipSpace[SHADOW_CASCADE_COUNT];
}


struct vs_in
{
	float2 position : POS;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float2 texcoord : TEX;
};

Texture2D    u_positionTexture : register(t0);
SamplerState u_positionSampler : register(s0);
Texture2D    u_albedoTexture : register(t0);
SamplerState u_albedoSampler : register(s0);
Texture2D    u_normalTexture : register(t1);
SamplerState u_normalSampler : register(s1);
Texture2D    u_depthTexture : register(t1);
SamplerState u_depthSampler : register(s1);
TextureCube  u_skyboxTexture : register(t1);
SamplerState u_skyboxSampler : register(s1);
//sampler2D u_shadowTexture[SHADOW_CASCADE_COUNT];

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.position = float4(input.position.x, input.position.y, 0, 1);
	output.texcoord = input.position * float2(0.5, 0.5) + float2(0.5, 0.5);
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	float4 albedo = u_albedoTexture.Sample(u_albedoSampler, input.texcoord);
	float4 normal = u_albedoTexture.Sample(u_albedoSampler, input.texcoord);
	float4 position = u_albedoTexture.Sample(u_albedoSampler, input.texcoord);
	return albedo;
}
