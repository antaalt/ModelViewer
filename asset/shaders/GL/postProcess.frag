#version 330

layout(location = 0) out vec4 o_color;

in vec2 v_uv;

uniform sampler2D u_inputTexture;

layout(std140) uniform ViewportUniformBuffer {
	vec2 u_screen;
};


#define FXAA_SUBPIX_SHIFT (1.0/4.0)
#define FXAA_SPAN_MAX (8.0)
#define FXAA_REDUCE_MUL (1.0/8.0)
#define FXAA_REDUCE_MIN (1.0/128.0)

// http://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf
// https://www.geeks3d.com/20110405/fxaa-fast-approximate-anti-aliasing-demo-glsl-opengl-test-radeon-geforce/3/

vec3 fxaa()
{
	vec2 rcpFrame = vec2(1.0) / u_screen;
	vec2 pos = v_uv - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));

	// Color must be sRGB.
	vec3 rgbNW = textureLod(u_inputTexture, pos, 0.0).xyz;
	vec3 rgbNE = textureLod(u_inputTexture, pos + ivec2(1,0) * rcpFrame, 0.0).xyz;
	vec3 rgbSW = textureLod(u_inputTexture, pos + ivec2(0,1) * rcpFrame, 0.0).xyz;
	vec3 rgbSE = textureLod(u_inputTexture, pos + ivec2(1,1) * rcpFrame, 0.0).xyz;
	vec3 rgbM  = textureLod(u_inputTexture, v_uv, 0.0).xyz;

	vec3 luma = vec3(0.299, 0.587, 0.114);
	float lumaNW = dot(rgbNW, luma);
	float lumaNE = dot(rgbNE, luma);
	float lumaSW = dot(rgbSW, luma);
	float lumaSE = dot(rgbSE, luma);
	float lumaM  = dot(rgbM,  luma);

	float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
	float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

	vec2 dir;
	dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
	dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

	float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL), FXAA_REDUCE_MIN);
	float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);

	dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) * rcpFrame.xy;

	vec3 rgbA = (1.0/2.0) * (textureLod(u_inputTexture, v_uv + dir * (1.0/3.0 - 0.5), 0.0).xyz + textureLod(u_inputTexture, v_uv + dir * (2.0/3.0 - 0.5), 0.0).xyz);
	vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (textureLod(u_inputTexture, v_uv + dir * (0.0/3.0 - 0.5), 0.0).xyz + textureLod(u_inputTexture, v_uv + dir * (3.0/3.0 - 0.5), 0.0).xyz);

	float lumaB = dot(rgbB, luma);

	if((lumaB < lumaMin) || (lumaB > lumaMax))
		return rgbA;
	else
		return rgbB;
}

vec3 ACESFilm(vec3 x)
{
	const float a = 2.51f;
	const float b = 0.03f;
	const float c = 2.43f;
	const float d = 0.59f;
	const float e = 0.14f;
	return vec3(clamp((x*(a*x+b))/(x*(c*x+d)+e), 0, 1));
}

void main()
{
	vec3 antialiased = fxaa();
	// Tonemapping
	// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
	vec3 color = ACESFilm(antialiased); // ACES approximation tonemapping
	//color = color / (color + vec3(1.0)); // Reinhard operator

	// Gamma correction
	color = pow(color, vec3(1.0/2.2));

	// Output
	o_color = vec4(color, 1.0);
}
