cbuffer constants : register(b0)
{
	float4x4 u_light;
	float4x4 u_model;
}

struct vs_in
{
	float3 position : POS;
	float3 normal : NORM;
	float2 tex : TEX;
	float4 color : COL;
};

struct vs_out
{
	float4 position : SV_POSITION;
};

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.position = mul(u_light, mul(u_model, float4(input.position, 1.0f)));
	return output;
}
void ps_main(vs_out input) {}
