cbuffer constants : register(b0)
{
	row_major float4x4 u_light;
	row_major float4x4 u_model;
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
	output.position = mul(mul(float4(input.position, 1.0f), u_model), u_light);
	return output;
}
float4 ps_main(vs_out input) : SV_TARGET
{
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
}
