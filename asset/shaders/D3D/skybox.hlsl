cbuffer constants : register(b0)
{
	row_major float4x4 u_view;
	row_major float4x4 u_projection;
}

struct vs_in
{
	float3 position : POS;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float3 texcoord : TEX;
};

TextureCube  u_skyboxTexture : register(t0);
SamplerState u_skyboxSampler : register(s0);

vs_out vs_main(vs_in input)
{
	vs_out output;
	float4 p = mul(mul(float4(input.position, 1.0f), u_view), u_projection);
	output.position = float4(p.x, p.y, p.w, p.w);
	output.texcoord = input.position;
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	return u_skyboxTexture.Sample(u_skyboxSampler, input.texcoord);
}
