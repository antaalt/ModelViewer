cbuffer constants : register(b0)
{
	float4x4 u_view;
	float4x4 u_projection;
}

struct vs_out
{
	float4 position : SV_POSITION;
	float3 texcoord : TEX;
};

TextureCube  u_skyboxTexture : register(t0);
SamplerState u_skyboxSampler : register(s0);

vs_out vs_main(float3 position : POS)
{
	vs_out output;
	float4 p = mul(u_projection, mul(u_view, float4(position, 1.0f)));
	output.position = p.xyww;
	output.texcoord = position;
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	return u_skyboxTexture.Sample(u_skyboxSampler, input.texcoord);
}
