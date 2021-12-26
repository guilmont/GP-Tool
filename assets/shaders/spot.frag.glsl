#version 410 core

in vec2 fTexCoord;
in vec4 fColor;
in vec2 fPos;
in float fRadius;
in float fSelected;

out vec4 fragColor;

uniform float u_thickness;

void main()
{
    float dist = distance(fPos, fTexCoord);

    float 
        r0 = (1.0 - 1.5 * u_thickness) * fRadius,
        r1 = (1.0 - 1.0 * u_thickness) * fRadius,
        r2 = fRadius,
        r3 = (1.0 + 0.5 * u_thickness) * fRadius;

    float 
        c0 = smoothstep(r0 + 0.0001, r0, dist),
        c1 = smoothstep(r1 + 0.0001, r1, dist),
        c2 = smoothstep(r2 + 0.0001, r2, dist),
        c3 = smoothstep(r3 + 0.0001, r3, dist);


    vec3 color = fSelected * (c3 - c2 + c1 - c0) * vec3(1.0, 1.0, 1.0) + (c2 - c1) * fColor.rgb;
    float alpha = fSelected * (c3 - c0) + (1.0 - fSelected) * (c2 - c1);

    fragColor = vec4(color, alpha);
}