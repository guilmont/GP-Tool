#version 410 core

uniform vec4 color;
uniform int nPoints;
uniform vec2 u_points[20];

// Input :: Output
in vec2 fTexCoord;
out vec4 frag_color;


int drawLine (vec2 p1, vec2 p2, vec2 pos, float a)
{
    float d = distance(p1, p2);
    float dpos = distance(p1, pos);

    //if point is on line, according to dist, it should match current position
    return int(1.0-floor(1.0-a+distance (mix(p1, p2, clamp(dpos/d, 0.0, 1.0)), pos)));

} // drawLine

int crossLine(vec2 p1, vec2 p2, vec2 pos)
{
    vec2 dp = p2 - p1;
    mat2 oi = mat2(pos.x, pos.y, -dp.x, -dp.y);

    if (determinant(oi) == 0.0)
        return 0;

    vec2 su = inverse(oi) * p1;

    if (su.x >=0 && su.x <= 1.0 && su.y >= 0.0 && su.y <= 1.0)
        return 1;
    else
        return 0;

} // crossLine

void main()
{
    int inner = 0, lines = 0, dots = 0;
    for (int k = 0; k < nPoints; k++)
    {
        int p2 = (k+1) % nPoints;

        dots += int(distance(u_points[k], fTexCoord) < 0.002);
        lines += drawLine(u_points[k], u_points[p2], fTexCoord, 0.0005);
        inner += crossLine(u_points[k], u_points[p2], fTexCoord);
    }

    if (dots> 0)
        frag_color = vec4(color.rgb, 1.0);
   else if (lines>0)
        frag_color = vec4(0.7*color.rgb, 1.0);
    else if (inner % 2  == 0)
        frag_color = vec4(0.0);
    else
        frag_color = color;

} // main