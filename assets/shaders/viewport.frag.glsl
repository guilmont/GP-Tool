#version 410 core

// We certainly don't need more than 5
uniform int u_nChannels;
uniform vec3 u_color[5];
uniform sampler2D u_texture[5];

uniform vec2 u_size;
uniform mat3 u_align[5];

// Input :: Output                                                                          
in vec2 fragCoord;                                                                          
out vec4 fragColor;                                                                        
                                                                                            
void main()
{

    fragColor = vec4(0.0,0.0,0.0,1.0);
    for (int ch = 0; ch < u_nChannels; ch++)
    {
        vec2 nCoord = (u_align[ch] * vec3(fragCoord*u_size, 1.0)).xy/u_size;
        fragColor.rgb += u_color[ch] * texture(u_texture[ch], nCoord).x;
    }

                                                                                            
} // main
