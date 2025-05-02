#version 460 core 
    in vec4 aPosition; 
    in vec3 aNormal; 
    in vec4 aColor; 
    out vec4 oColor; 
    uniform mat4 uModelViewMatrix; 
    uniform mat4 uProjectionViewMatrix; 
    uniform vec3 uLd; 
    uniform vec3 uKd; 
    uniform vec4 uLightPosition; 
    uniform int ukeypressed; 
    out vec3 oDiffusedLight; 
    void main() 
    { 
        if(ukeypressed == 1) 
        { 
            vec4 iPosition = uModelViewMatrix * aPosition; 
            mat3 normalMatrix = mat3(transpose(inverse(uModelViewMatrix))); 
            vec3 n = normalize(normalMatrix * aNormal); 
            vec3 s = normalize(vec3(uLightPosition - iPosition)); 
            oDiffusedLight = uLd * uKd * dot(s, n); 
        } 
        else 
        {
            oDiffusedLight = vec3(0.0, 0.0, 0.0); 
        } 
            gl_Position = uProjectionViewMatrix * uModelViewMatrix * aPosition; 
        }
        