#version 430 core

layout(location = 0) in vec3 vPos;
layout(location = 1) in vec4 vColor;

out vec4 fColor;
uniform mat4 u_transform;

void main()
{
    fColor = vColor;
    gl_Position = u_transform * vec4(vPos, 1.0);
}
