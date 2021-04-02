#version 410 core

uniform vec3 color;
uniform float bHeight[256];

uniform float lowContrast;
uniform float highContrast;

// Input :: Output
in vec2 fTexCoord;
out vec4 frag_color;

void main()
{
    float top = 0.0;
    for(int k = 0; k < 256; k++)
        top = bHeight[k] > top ? bHeight[k] : top;

    top = 0.8/top;

    int bin = int(255.0*fTexCoord.x);
    float limit = top*bHeight[bin];

    vec3 cor;
    if(fTexCoord.y < limit)
    {
        if (color == vec3(1.0))
            cor = vec3(0.0);
        else
            cor = 0.85*color;

    }
    else
        cor = vec3(0.9);


    if (fTexCoord.x < lowContrast || fTexCoord.x > highContrast)
        cor *= 0.6;

    frag_color = vec4(cor, 1.0);

} // main