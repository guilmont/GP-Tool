#version 410 core

// We certainly don't need more than 5
uniform int u_nChannels;
uniform vec3 u_color[5];
uniform sampler2D u_texture[5];

uniform mat4 u_align;

// Input :: Output                                                                          
in vec2 fragCoord;                                                                          
out vec4 fragColor;                                                                        
                                                                                            
void main()                                                                                 
{   

    fragColor = vec4(0.0,0.0,0.0,1.0);
    for (int ch = 0; ch < u_nChannels; ch++)
        fragColor.rgb += u_color[ch] * texture(u_texture[ch], fragCoord).x;

                                                                                           
} // main