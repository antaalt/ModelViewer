cbuffer constants : register(b0)
{
	row_major float4x4 u_lights[3];
}

struct vs_in
{
	float2 position : POS;
};

struct vs_out
{
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.uv = input.position * float2(0.5, 0.5) + float2(0.5, 0.5);
	output.position = float4(input.position, 0.0, 1.0);
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	// TODO shading
	return float4(0.0f,0.f,0.1f,1.f);
}
