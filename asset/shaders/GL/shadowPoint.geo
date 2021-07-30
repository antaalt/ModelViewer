#version 330 core
layout (triangles) in;
layout (triangle_strip, max_vertices=18) out;

uniform mat4 u_lights[6];

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
