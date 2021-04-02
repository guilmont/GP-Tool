#version 410 core

in vec2 fragCoord;
out vec4 fragColor;

void main()
{
    vec2 uv = fragCoord - 0.5;

    float cor = 0;

    vec2 pos[4];
    pos[0] = vec2(-0.3,0.3);    
    pos[1] = vec2(-0.3,-0.3);    
    pos[2] = vec2(0.3,-0.3);    
    pos[3] = vec2(0.3,0.3);    


    for(int k =0; k < 4; k++)
    {
        float dist = length(uv + pos[k]);
        cor += smoothstep(0.1, 0.099, dist);
    }

    vec4 bg = vec4(0.2, 0.5, 0.9,1.0);
    vec4 ball = vec4(0.9, 0.5, 0.2,1.0);

    fragColor = mix(bg, ball, cor);
}