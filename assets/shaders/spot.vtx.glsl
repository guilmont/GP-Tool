#version 410 core

layout(location = 0) in vec4 quadPosition;
layout(location = 1) in vec2 quadTexCoord;
layout(location = 2) in vec4 circleColor;
layout(location = 3) in vec2 circlePosition;
layout(location = 4) in float circleRadius;

out vec2 fTexCoord;
out vec4 fColor;
out vec2 fPos;
out float fRadius;

uniform mat4 u_transform;

void main()
{
    gl_Position = u_transform * quadPosition;

    fTexCoord = quadTexCoord;
    fColor = circleColor;
    fPos = circlePosition;
    fRadius = circleRadius;
}
