cbuffer constants : register(b0)
{
	uint u_width;
	uint u_height;
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

Texture2D    u_inputTexture : register(t0);
SamplerState u_inputSampler : register(s0);

float3 ACESFilm(float3 x)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return saturate((x*(a*x+b))/(x*(c*x+d)+e));
}

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.texcoord = input.position * float2(0.5, 0.5) + float2(0.5, 0.5);
	output.position = float4(input.position.x, input.position.y, 0.0, 1.0);
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	// TODO fxaa
	float4 color4 = u_inputTexture.Sample(u_inputSampler, input.texcoord);
	float3 color = float3(color4.x, color4.y, color4.z);

	// Tonemapping
	color = ACESFilm(color);

	// Gamma correction
	color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));

	return float4(color, 1.0);
}
