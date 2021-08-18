cbuffer constants : register(b0)
{
	float4x4 u_mvp;
}

struct vs_in
{
	float3 position : POS;
};

float4 vs_main(vs_in input) : SV_POSITION
{
	return mul(u_mvp, float4(input.position, 1.0));
}

float4 ps_main(float4 input : SV_POSITION) : SV_TARGET
{
	// TODO shading
	return float4(0.1f,0.f,0.f,1.f);
}
