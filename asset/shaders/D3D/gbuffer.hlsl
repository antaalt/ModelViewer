cbuffer constants : register(b0)
{
	row_major float4x4 u_model;
	row_major float4x4 u_view;
	row_major float4x4 u_projection;
	row_major float3x3 u_normalMatrix;
	float4 u_color;
}


struct vs_in
{
	float3 position : POS;
	float3 normal : NORM;
	float2 texcoord : TEX;
	float4 color : COL;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float3 normal : NORM;
	float2 texcoord : TEX;
	float4 color : COL;
};

struct PS_OUTPUT
{
	float4 position: SV_Target0;
	float4 albedo: SV_Target1;
	float4 normal: SV_Target2;
};

Texture2D    u_colorTexture : register(t0);
SamplerState u_colorSampler : register(s0);
Texture2D    u_normalTexture : register(t1);
SamplerState u_normalSampler : register(s1);

vs_out vs_main(vs_in input)
{
	vs_out output;

	output.position = mul(mul(mul(float4(input.position, 1.0f), u_model), u_view), u_projection);
	output.normal = input.normal;
	// normalize(mul(u_normalMatrix, input.normal));
	output.texcoord = input.texcoord;
	output.color = input.color * u_color;

	return output;
}

PS_OUTPUT ps_main(vs_out input)
{
	PS_OUTPUT output;
	output.albedo = input.color * u_colorTexture.Sample(u_colorSampler, input.texcoord);
	// TODO normal mapping
	output.normal = float4(input.normal.x, input.normal.y, input.normal.z, 1);//u_normalTexture.Sample(u_normalSampler, input.texcoord);
	output.position = input.position;
	return output;
}
