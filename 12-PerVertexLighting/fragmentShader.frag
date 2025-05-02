#version 460 core
        in vec4 oColor;
        in vec3 oPhong_ADS_Light;
        uniform int ukeypressed;
        out vec4 FragColor;
        void main()
        {
        if(ukeypressed == 1)
        {
        FragColor = vec4(oPhong_ADS_Light, 1.0);
        }
        else
        {
        FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
        }
