cbuffer ViewportUniformBuffer : register(b0)
{
	float2 u_screen;
};
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

#define FXAA_SUBPIX_SHIFT (1.0/4.0)
#define FXAA_SPAN_MAX (8.0)
#define FXAA_REDUCE_MUL (1.0/8.0)
#define FXAA_REDUCE_MIN (1.0/128.0)

// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
// https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/

float3 fxaa(float2 texcoord)
{
	float2 rcpFrame = float2(1.0, 1.0) / u_screen;
	float2 pos = texcoord - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));

	// Color must be sRGB.
	float3 rgbNW = u_inputTexture.SampleLevel(u_inputSampler, pos, 0.0).xyz;
	float3 rgbNE = u_inputTexture.SampleLevel(u_inputSampler, pos + int2(1,0) * rcpFrame, 0.0).xyz;
	float3 rgbSW = u_inputTexture.SampleLevel(u_inputSampler, pos + int2(0,1) * rcpFrame, 0.0).xyz;
	float3 rgbSE = u_inputTexture.SampleLevel(u_inputSampler, pos + int2(1,1) * rcpFrame, 0.0).xyz;
	float3 rgbM  = u_inputTexture.SampleLevel(u_inputSampler, texcoord, 0.0).xyz;

	float3 luma = float3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	float2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

	dir = min(float2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), max(float2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * rcpFrame.xy;

	float3 rgbA = (1.0/2.0) * (u_inputTexture.SampleLevel(u_inputSampler, texcoord + dir * (1.0/3.0 - 0.5), 0.0).xyz + u_inputTexture.SampleLevel(u_inputSampler, texcoord + dir * (2.0/3.0 - 0.5), 0.0).xyz);
	float3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (u_inputTexture.SampleLevel(u_inputSampler, texcoord + dir * (0.0/3.0 - 0.5), 0.0).xyz + u_inputTexture.SampleLevel(u_inputSampler, texcoord + dir * (3.0/3.0 - 0.5), 0.0).xyz);

	float lumaB = dot(rgbB, luma);

	if((lumaB < lumaMin) || (lumaB > lumaMax))
		return rgbA;
	else
		return rgbB;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	float3 antialiased = fxaa(input.texcoord);
	// Tonemapping
	float3 color = ACESFilm(antialiased);

	// Gamma correction
	color = pow(color, float3(1.0/2.2, 1.0/2.2, 1.0/2.2));

	return float4(color, 1.0);
}
