#version 410 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec2 vTexCoord;

uniform mat4 u_transform;

out vec2 fragCoord;

void main()
{
    fragCoord = vTexCoord;
    gl_Position = u_transform * vec4(vPos, 1.0);
} 
