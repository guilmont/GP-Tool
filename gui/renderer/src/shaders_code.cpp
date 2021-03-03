#pragma once

const char *basic_vert =
    "#version 410 core                                \n"
    "                                                 \n"
    "// Accessing buffer in global gpu memory         \n"
    "layout(location = 0) in vec3 vPos;               \n"
    "layout(location = 1) in vec2 vTexCoord;          \n"
    "                                                 \n"
    "uniform mat4 u_transform;                        \n"
    "                                                 \n"
    "// Outputs to fragment shader                    \n"
    "out vec2 fTexCoord;                              \n"
    "                                                 \n"
    "void main()                                      \n"
    "{                                                \n"
    "                                                 \n"
    "    fTexCoord = vTexCoord;                       \n"
    "    gl_Position = u_transform * vec4(vPos, 1.0); \n"
    "                                                 \n"
    "} // main                                        \n";

const char *histogram_frag =
    "  #version 410 core                                               \n"
    "                                                                  \n"
    "uniform vec3 color;                                               \n"
    "uniform float bHeight[256];                                       \n"
    "                                                                  \n"
    "uniform float lowContrast;                                        \n"
    "uniform float highContrast;                                       \n"
    "                                                                  \n"
    "// Input :: Output                                                \n"
    "in vec2 fTexCoord;                                                \n"
    "out vec4 frag_color;                                              \n"
    "                                                                  \n"
    "void main()                                                       \n"
    "{                                                                 \n"
    "    float top = 0.0;                                              \n"
    "    for(int k = 0; k < 256; k++)                                  \n"
    "        top = bHeight[k] > top ? bHeight[k] : top;                \n"
    "                                                                  \n"
    "    top = 0.8/top;                                                \n"
    "                                                                  \n"
    "    int bin = int(255.0*fTexCoord.x);                             \n"
    "    float limit = top*bHeight[bin];                               \n"
    "                                                                  \n"
    "    vec3 cor;                                                     \n"
    "    if(fTexCoord.y < limit)                                       \n"
    "    {                                                             \n"
    "        if (color == vec3(1.0))                                   \n"
    "            cor = vec3(0.0);                                      \n"
    "        else                                                      \n"
    "            cor = 0.85*color;                                     \n"
    "                                                                  \n"
    "    }                                                             \n"
    "    else                                                          \n"
    "        cor = vec3(0.9);                                          \n"
    "                                                                  \n"
    "                                                                  \n"
    "    if (fTexCoord.x < lowContrast || fTexCoord.x > highContrast)  \n"
    "        cor *= 0.6;                                               \n"
    "                                                                  \n"
    "    frag_color = vec4(cor, 1.0);                                  \n"
    "                                                                  \n"
    "} // main                                                         \n";

const char *selectRoi_frag =
    "    #version 410 core                                                                   \n"
    "                                                                                        \n"
    "uniform vec4 color;                                                                     \n"
    "uniform int nPoints;                                                                    \n"
    "uniform vec2 u_points[20];                                                              \n"
    "                                                                                        \n"
    "// Input :: Output                                                                      \n"
    "in vec2 fTexCoord;                                                                      \n"
    "out vec4 frag_color;                                                                    \n"
    "                                                                                        \n"
    "                                                                                        \n"
    "int drawLine (vec2 p1, vec2 p2, vec2 pos, float a)                                      \n"
    "{                                                                                       \n"
    "    float d = distance(p1, p2);                                                         \n"
    "    float dpos = distance(p1, pos);                                                     \n"
    "                                                                                        \n"
    "    //if point is on line, according to dist, it should match current position          \n"
    "    return int(1.0-floor(1.0-a+distance (mix(p1, p2, clamp(dpos/d, 0.0, 1.0)), pos)));  \n"
    "                                                                                        \n"
    "} // drawLine                                                                           \n"
    "                                                                                        \n"
    "int crossLine(vec2 p1, vec2 p2, vec2 pos)                                               \n"
    "{                                                                                       \n"
    "    vec2 dp = p2 - p1;                                                                  \n"
    "    mat2 oi = mat2(pos.x, pos.y, -dp.x, -dp.y);                                         \n"
    "                                                                                        \n"
    "    if (determinant(oi) == 0.0)                                                         \n"
    "        return 0;                                                                       \n"
    "                                                                                        \n"
    "    vec2 su = inverse(oi) * p1;                                                         \n"
    "                                                                                        \n"
    "    if (su.x >=0 && su.x <= 1.0 && su.y >= 0.0 && su.y <= 1.0)                          \n"
    "        return 1;                                                                       \n"
    "    else                                                                                \n"
    "        return 0;                                                                       \n"
    "                                                                                        \n"
    "} // crossLine                                                                          \n"
    "                                                                                        \n"
    "void main()                                                                             \n"
    "{                                                                                       \n"
    "    int inner = 0, lines = 0, dots = 0;                                                 \n"
    "    for (int k = 0; k < nPoints; k++)                                                   \n"
    "    {                                                                                   \n"
    "        int p2 = (k+1) % nPoints;                                                       \n"
    "                                                                                        \n"
    "        dots += int(distance(u_points[k], fTexCoord) < 0.002);                          \n"
    "        lines += drawLine(u_points[k], u_points[p2], fTexCoord, 0.0005);                \n"
    "        inner += crossLine(u_points[k], u_points[p2], fTexCoord);                       \n"
    "    }                                                                                   \n"
    "                                                                                        \n"
    "    if (dots> 0)                                                                        \n"
    "        frag_color = vec4(color.rgb, 1.0);                                              \n"
    "   else if (lines>0)                                                                    \n"
    "        frag_color = vec4(0.7*color.rgb, 1.0);                                          \n"
    "    else if (inner % 2  == 0)                                                           \n"
    "        frag_color = vec4(0.0);                                                         \n"
    "    else                                                                                \n"
    "        frag_color = color;                                                             \n"
    "                                                                                        \n"
    "} // main                                                                               \n";

const char *trajectory_frag =
    "#version 410 core                          \n"
    "                                           \n"
    "uniform vec3 color;                        \n"
    "uniform vec2 position;                     \n"
    "uniform float radius;                      \n"
    "                                           \n"
    "// Input :: Output                         \n"
    "in vec2 fTexCoord;                         \n"
    "out vec4 frag_color;                       \n"
    "                                           \n"
    "void main()                                \n"
    "{                                          \n"
    "   float r = length(fTexCoord - position), \n"
    "          top = radius + 0.0005,           \n"
    "          bot = radius - 0.0005;           \n"
    "                                           \n"
    "                                           \n"
    "    if (r > bot && r < top)                \n"
    "        frag_color = vec4(color, 1.0);     \n"
    "    else                                   \n"
    "        frag_color = vec4(0.0);            \n"
    "                                           \n"
    "                                           \n"
    "} // main                                  \n";

const char *viewport_frag =
    "#version 410 core                                                                           \n"
    "                                                                                            \n"
    "uniform bool clear;                                                                         \n"
    "uniform vec3 colormap;                                                                      \n"
    "uniform sampler2D u_signal;                                                                 \n"
    "                                                                                            \n"
    "uniform vec2 dim;                                                                           \n"
    "uniform mat3 u_align;                                                                       \n"
    "                                                                                            \n"
    "// Input :: Output                                                                          \n"
    "in vec2 fTexCoord;                                                                          \n"
    "out vec4 frag_color;                                                                        \n"
    "                                                                                            \n"
    "void main()                                                                                 \n"
    "{                                                                                           \n"
    "                                                                                            \n"
    "    if (clear)                                                                              \n"
    "       frag_color = vec4(0.0, 0.0, 0.0, 1.0);                                               \n"
    "                                                                                            \n"
    "    else                                                                                    \n"
    "    {                                                                                       \n"
    "        vec2 oldCoord = dim * fTexCoord;                                                    \n"
    "        vec2 coord;                                                                         \n"
    "        coord.x = u_align[0][0] * oldCoord.x + u_align[0][1] * oldCoord.y + u_align[0][2];  \n"
    "                                                                                            \n"
    "        coord.y = u_align[1][0] * oldCoord.x + u_align[1][1] * oldCoord.y + u_align[1][2];  \n"
    "                                                                                            \n"
    "        coord /= dim;                                                                       \n"
    "                                                                                            \n"
    "        float sig =  texture(u_signal, coord).x;                                            \n"
    "        frag_color = vec4(sig * colormap,  1.0);                                            \n"
    "    }                                                                                       \n"
    "                                                                                            \n"
    "} // main                                                                                   \n";