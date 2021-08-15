cbuffer constants : register(b0)
{
	row_major float4x4 u_lights[6];
	row_major float4x4 u_model;
	float3 u_lightPos;
	float u_far;
}

struct gs_out
{
	float4 lightSpacePosition : SV_POSITION;
	float4 worldPosition : POSITION;
	uint face : SV_RenderTargetArrayIndex;
};

float4 vs_main(float3 position : POS) : SV_POSITION
{
	return mul(float4(position, 1.0f), u_model);
}

[maxvertexcount(18)]
void gs_main(triangle float4 input[3] : SV_POSITION, inout TriangleStream<gs_out> stream)
{
	for (int iFace = 0; iFace < 6; ++iFace)
	{
		gs_out output;
		output.face = iFace;
		for (int iVert = 0; iVert < 3; ++iVert)
		{
			output.worldPosition = input[iVert];
			output.lightSpacePosition = mul(input[iVert], u_lights[iFace]);
			stream.Append(output);
		}
		stream.RestartStrip();
	}
}

float ps_main(gs_out input) : SV_Depth
{
	// get distance between fragment and light source
	float3 wPos = float3(input.worldPosition.x, input.worldPosition.y, input.worldPosition.z);
	float lightDistance = length(wPos - u_lightPos);

	// map to [0;1] range by dividing by far_plane
	lightDistance = lightDistance / u_far;

	// write this as modified depth
	return lightDistance;
}
