#version 410 core

// Movie plugin :: We certainly don't need more than 5
uniform int u_nChannels;
uniform vec2 u_size;
uniform vec3 u_color[5];
uniform sampler2D u_texture[5];

// Align plugin 
uniform mat3 u_align[5];

// Trajectory plugin
uniform int u_nPoints;
uniform vec3 u_ptPos[128];
uniform vec3 u_ptColor[128];

// Input from vertex shader
in vec2 fragCoord;
out vec4 fragColor;


void main()
{
    
    // Main image and alignment
    fragColor = vec4(0.0,0.0,0.0,1.0);

    if (u_nChannels == 1)
    {
        fragColor.rgb = u_color[0] * texture(u_texture[0], fragCoord).x;
    }
    else
    {
        for (int ch = 0; ch < u_nChannels; ch++)
        {
            vec2 nCoord = (u_align[ch] * vec3(fragCoord*u_size, 1.0)).xy/u_size;
            fragColor.rgb += u_color[ch] * texture(u_texture[ch], nCoord).x;
        }
    }

    ///////////////////////////////////////////////////////
    // Spots

    float dr = 0.0007; // rim thickness           
    for (int pt = 0; pt < u_nPoints; pt++)
    {
        float rad = u_ptPos[pt].z;
        float r = length(fragCoord - u_ptPos[pt].xy/u_size);
        float w = smoothstep(rad+dr+0.0001,rad+dr, r)
                - smoothstep(rad-dr, rad - dr-0.0001, r);

        fragColor.rgb = mix(fragColor.rgb, u_ptColor[pt], w);
    }
   
    ///////////////////////////////////////////////////////
    // ROI

} // main
