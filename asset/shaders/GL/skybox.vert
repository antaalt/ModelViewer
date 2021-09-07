#version 330 core
layout (location = 0) in vec3 a_position;

out vec3 v_uv;

layout(std140) uniform CameraUniformBuffer {
    mat4 u_view;
    mat4 u_projection;
};

void main()
{
    v_uv = a_position;
    // We only need rotation of camera.
    vec4 pos = u_projection * vec4(mat3(u_view) * a_position, 1.0);
    gl_Position = pos.xyww;
}
