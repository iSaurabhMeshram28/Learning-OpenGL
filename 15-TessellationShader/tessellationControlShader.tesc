#version 460 core
layout(vertices=4) out;
uniform int uNumberOfSegments;
uniform int uNumberOfStrips;
void main()
{
        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
        gl_TessLevelOuter[0] = float(uNumberOfStrips);
        gl_TessLevelOuter[1] = float(uNumberOfSegments);
}
