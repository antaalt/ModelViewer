cbuffer CameraUniformBuffer : register(b0)
{
	float4x4 u_view;
	float4x4 u_projection;
	float4x4 u_viewInverse;
	float4x4 u_projectionInverse;
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


Texture2D    u_positionTexture;
SamplerState u_positionSampler;
Texture2D    u_albedoTexture;
SamplerState u_albedoSampler;
Texture2D    u_normalTexture;
SamplerState u_normalSampler;
Texture2D    u_materialTexture;
SamplerState u_materialSampler;
TextureCube  u_skyboxTexture;
SamplerState u_skyboxSampler;

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.texcoord = input.position * float2(0.5, 0.5) + float2(0.5, 0.5);
	output.position = float4(input.position, 0.0, 1.0);
	output.texcoord.y = 1.f - output.texcoord.y;
	return output;
}

float4 ps_main(vs_out input) : SV_TARGET
{
	float4 position = u_positionTexture.Sample(u_positionSampler, input.texcoord);
	float4 normal   = u_normalTexture.Sample(u_normalSampler, input.texcoord);
	float4 albedo   = pow(u_albedoTexture.Sample(u_albedoSampler, input.texcoord), float4(2.2, 2.2, 2.2, 1.0)); // To Linear space
	float4 material = u_materialTexture.Sample(u_materialSampler, input.texcoord); // AO / roughness / metalness
	float ao = material.r;


	float3 N = normalize(float3(normal.x, normal.y, normal.z));
	float3 V = normalize(float3(u_viewInverse[0][0], u_viewInverse[0][1], u_viewInverse[0][2]) - float3(position.x, position.y, position.z));
	float3 I = -V;

	// Reflection
	float4 reflection = u_skyboxTexture.Sample(u_skyboxSampler, reflect(I, N));

	// Basic ambient shading
	float3 indirect = 0.3 * float3(albedo.x, albedo.y, albedo.z); // ao broken for some
	float3 color = indirect + float3(reflection.x, reflection.y, reflection.z) * 0.01; // TODO use irradiance map

	return float4(color.x, color.y, color.z, 1.0);
}
