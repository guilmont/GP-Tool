#version 410 core

uniform vec3 color;
uniform vec2 position;
uniform float radius;

// Input :: Output
in vec2 fTexCoord;
out vec4 frag_color;

void main()
{
   float r = length(fTexCoord - position),
          top = radius + 0.0005,
          bot = radius - 0.0005;


    if (r > bot && r < top)
        frag_color = vec4(color, 1.0);
    else
        frag_color = vec4(0.0);


} // main