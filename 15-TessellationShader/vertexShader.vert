#version 460 core
in vec2 aPosition;
void main()
{
        gl_Position= vec4(aPosition, 0.0, 1.0);
}
