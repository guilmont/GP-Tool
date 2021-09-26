#version 430 core

// ROI display
uniform vec4 roiColor;
uniform int nPoints;
uniform vec2 vecRoi[20];


in vec4 fColor;
out vec4 fragColor;

void main()
{
    fragColor = fColor;
}

