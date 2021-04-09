#version 410 core

uniform vec3 color;
uniform vec2 contrast;
uniform float histogram[256];

// Input :: Output
in vec2 fragCoord;
out vec4 fragColor;

void main()
{
    float top = 0.0;
    for(int k = 0; k < 256; k++)
        top = histogram[k] > top ? histogram[k] : top;

    top = 0.8/top;

    int bin = int(255.0*fragCoord.x);
    float limit = top*histogram[bin];

    fragColor = vec4(0.8,0.8,0.8,1.0);
    if(fragCoord.y < limit)
    {
        if (color == vec3(1.0)) // white would be hard to see
            fragColor.rgb = vec3(0.0);
        else
            fragColor.rgb = 0.85*color;

    }


    // Darker color for contrast bars
    if (fragCoord.x < contrast.x || fragCoord.x > contrast.y)
        fragColor.rgb *= 0.6;

} // main