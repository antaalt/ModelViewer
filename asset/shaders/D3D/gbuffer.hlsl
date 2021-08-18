cbuffer constants : register(b0)
{
	float4x4 u_model;
	float4x4 u_view;
	float4x4 u_projection;
	float3x3 u_normalMatrix;
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

struct ps_out
{
	float4 position: SV_Target0;
	float4 albedo: SV_Target1;
	float4 normal: SV_Target2;
	float4 material: SV_Target3;
};

Texture2D    u_colorTexture;
SamplerState u_colorSampler;
Texture2D    u_normalTexture;
SamplerState u_normalSampler;
Texture2D    u_materialTexture;
SamplerState u_materialSampler;

vs_out vs_main(vs_in input)
{
	vs_out output;
	output.position = mul(u_projection, mul(u_view, mul(u_model, float4(input.position, 1.0f))));
	output.normal = normalize(mul(u_normalMatrix, input.normal));
	output.texcoord = input.texcoord;
	output.color = input.color * u_color;
	return output;
}

ps_out ps_main(vs_out input)
{
	// --- Generate albedo
	float4 albedo = input.color * u_colorTexture.Sample(u_colorSampler, input.texcoord);

	// --- Generate normals
	// Compute TBN matrix.
	// derivations of the fragment position
	float3 p_dx = ddx(float3(input.position.x, input.position.y, input.position.z));
	float3 p_dy = ddy(float3(input.position.x, input.position.y, input.position.z));
	// derivations of the texture coordinate
	float2 t_dx = ddx(input.texcoord);
	float2 t_dy = ddy(input.texcoord);
	// tangent vector and binormal vector
	float3 n = normalize(input.normal);
	float3 t = normalize(t_dy.y * p_dx - t_dx.y * p_dy);
	float3 b = normalize(t_dx.x * p_dy - t_dy.x * p_dx);
	// Gran-Schmidt method
	t = t - n * dot( t, n ); // orthonormalization ot the tangent vectors
	b = b - n * dot( b, n ); // orthonormalization of the binormal vectors to the normal vector
	b = b - t * dot( b, t ); // orthonormalization of the binormal vectors to the tangent vector
	float3x3 tbn = float3x3(t, b, n);
	float4 normal4 = u_normalTexture.Sample(u_normalSampler, input.texcoord);
	float3 normal = float3(normal4.x, normal4.y, normal4.z);
	normal = normal * 2.0 - 1.0;
	normal = normalize(mul(normal, tbn));

	// --- Alpha
	//if (bool(albedo.a < 0.8)) { // TODO use threshold
	//	discard;
	//}

	ps_out output;
	output.position = input.position;
	output.normal = float4(normal.x, normal.y, normal.z, 1);
	output.albedo = albedo;
	output.material = u_materialTexture.Sample(u_materialSampler, input.texcoord);
	return output;
}
