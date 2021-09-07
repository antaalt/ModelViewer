#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

layout(std140) uniform PointLightUniformBuffer {
	mat4 u_lights[6];
	vec3 u_lightPos;
	float u_far;
};

out vec4 v_position;

void main()
{
    for(int iFace = 0; iFace < 6; ++iFace)
    {
        gl_Layer = iFace;
        for(int iVert = 0; iVert < 3; ++iVert) // for each triangle vertex
        {
            v_position = gl_in[iVert].gl_Position;
            gl_Position = u_lights[iFace] * v_position;
            EmitVertex();
        }
        EndPrimitive();
    }
}
