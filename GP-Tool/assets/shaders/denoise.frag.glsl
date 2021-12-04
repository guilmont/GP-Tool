#version 410 core

in vec2 fragCoord;
out vec4 fragColor;

uniform sampler2D u_texture;

void main()
{
    float value = texture(u_texture, fragCoord).x;
    fragColor = vec4(value, value, value, 1);
}
