#version 410 core

in vec2 fTexCoord;
in vec4 fColor;
in vec2 fPos;
in float fRadius;

out vec4 fragColor;

uniform float u_thickness;

void main()
{
    float dist = distance(fPos, fTexCoord);
    float alpha = smoothstep(fRadius + 0.0001, fRadius, dist);

    float radius = fRadius - u_thickness * fRadius;
    alpha -= smoothstep(radius + 0.0001, radius, dist);

    fragColor = vec4(fColor.rgb, alpha);
}