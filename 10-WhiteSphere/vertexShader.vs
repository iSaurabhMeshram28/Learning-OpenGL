 #version 460 core 
    in vec4 aPosition; 
    in vec4 aColor; 
    out vec4 oColor; 
    uniform mat4 uModelViewMatrix; 
    uniform mat4 uProjectionViewMatrix; 
    void main() 
    { 
        gl_Position = uProjectionViewMatrix * uModelViewMatrix * aPosition; 
    }
    