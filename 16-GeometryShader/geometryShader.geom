#version 460 core
layout(triangles) in;
layout(triangle_strip, max_vertices = 9) out;
uniform mat4 uMVPMatrix;
void main()
{
    for(int i = 0; i < 3; i++)
    {
        gl_Position = uMVPMatrix * (gl_in[i].gl_Position + vec4(0.0, 1.0, 0.0, 0.0));
        EmitVertex();
        gl_Position = uMVPMatrix * (gl_in[i].gl_Position + vec4(-1.0, -1.0, 0.0, 0.0));
        EmitVertex();
        gl_Position = uMVPMatrix * (gl_in[i].gl_Position + vec4(1.0, -1.0, 0.0, 0.0));
        EmitVertex();
        EndPrimitive();
    }
}