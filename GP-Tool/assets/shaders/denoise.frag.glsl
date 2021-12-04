#version 410 core

uniform vec2 u_contrast;
uniform sampler2D u_texture;

// Input from vertex shader
in vec2 fragCoord;
out vec4 fragColor;

void main()
{
    float value = (texture(u_texture, fragCoord).x - u_contrast.x) / (u_contrast.y - u_contrast.x);
    fragColor = vec4(value, value, value, 1);
}
