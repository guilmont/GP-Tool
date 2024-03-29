#version 410 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec4 vColor;
layout(location = 2) in vec2 vTexCoord;
layout(location = 3) in float vTexID;

uniform mat4 u_transform;

out vec2 fragCoord;

void main()
{
    fragCoord = vTexCoord;
    gl_Position = u_transform * vec4(vPos, 1.0);
} 
