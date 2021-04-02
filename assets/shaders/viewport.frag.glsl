#version 410 core


uniform bool clear;                                                                         
uniform vec3 colormap;                                                                      
uniform sampler2D u_signal;                                                                 
                                                                                            
uniform vec2 dim;                                                                           
uniform mat3 u_align;                                                                       
                                                                                            
// Input :: Output                                                                          
in vec2 fragCoord;                                                                          
out vec4 frag_color;                                                                        
                                                                                            
void main()                                                                                 
{                                                                                           
                                                                                            
    if (clear)                                                                              
        frag_color = vec4(0.0, 0.0, 0.0, 1.0);                                               
                                                                                            
    else                                                                                    
    {                                                                                       
        vec2 oldCoord = dim * fragCoord;                                                    
        vec2 coord;                                                                         
        coord.x = u_align[0][0] * oldCoord.x + u_align[0][1] * oldCoord.y + u_align[0][2];  
                                                                                            
        coord.y = u_align[1][0] * oldCoord.x + u_align[1][1] * oldCoord.y + u_align[1][2];  
                                                                                            
        coord /= dim;                                                                       
                                                                                            
        float sig =  texture(u_signal, coord).x;                                            
        frag_color = vec4(sig * colormap,  1.0);                                            
    }                                                                                       
                                                                                            
} // main