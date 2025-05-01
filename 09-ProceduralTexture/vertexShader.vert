#version 460 core
in vec4 aPosition;
in vec2 aTexCoord;
out vec2 oTexCoord;
uniform mat4 uMVPMatrix;
void main() {
    gl_Position = uMVPMatrix * aPosition;
    oTexCoord = aTexCoord;
}
