#version 410 core

// Movie plugin :: We certainly don't need more than 5
uniform int u_nChannels;
uniform vec2 u_size;
uniform vec3 u_color[5];
uniform sampler2D u_texture[5];

// Align plugin 
uniform mat3 u_align[5];

// Input from vertex shader
in vec2 fragCoord;
out vec4 fragColor;


void main()
{
    
    // Main image and alignment
    fragColor = vec4(0.0,0.0,0.0,1.0);

    if (u_nChannels == 1)
        fragColor.rgb = u_color[0] * texture(u_texture[0], fragCoord).x;

    else
    {
        for (int ch = 0; ch < u_nChannels; ch++)
        {
            vec2 nCoord = (u_align[ch] * vec3(fragCoord*u_size, 1.0)).xy/u_size;
            fragColor.rgb += u_color[ch] * texture(u_texture[ch], nCoord).x;
        }
    }

} // main
