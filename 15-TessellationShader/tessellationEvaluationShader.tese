#version 460 core
layout(isolines) in;
uniform mat4 uMVPMatrix;
void main()
{
        vec3 p0 = gl_in[0].gl_Position.xyz;
        vec3 p1 = gl_in[1].gl_Position.xyz;
        vec3 p2 = gl_in[2].gl_Position.xyz;
        vec3 p3 = gl_in[3].gl_Position.xyz;
        vec3 p = pow(1.0 - gl_TessCoord.x, 3.0) * p0 + 3.0 * pow(1.0 - gl_TessCoord.x, 2.0) * gl_TessCoord.x * p1 + 3.0 * (1.0 - gl_TessCoord.x) * pow(gl_TessCoord.x, 2.0) * p2 + pow(gl_TessCoord.x, 3.0) * p3;
        gl_Position = uMVPMatrix * vec4(p, 1.0);
}
