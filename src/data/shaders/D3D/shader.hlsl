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

float4 ps_main(vs_out input) : SV_TARGET
{
	float4 normal4 = u_normalTexture.Sample(u_normalSampler, input.texcoord);
	float3 normal = float3(normal4.x, normal4.y, normal4.z);
	float cosTheta = clamp(dot(normal, normalize(float3(0,1,0))), 0.2, 1);
	float4 color = input.color * u_colorTexture.Sample(u_colorSampler, input.texcoord);
	return float4(color.rgb * cosTheta, color.a);
}
