#pragma once

#include "gpch.h"

class Circle
{

public:
    Circle(uint32_t numCircles);
    ~Circle(void);

    float thickness = 0.25f; // ratio of the radius

    void draw(const glm::vec2& position, float radius, const glm::vec4& color);
    void submit(void);

private: // Setup for quad
    struct QuadVertex
    {
        glm::vec3 position;
        glm::vec2 texCoord;
    };

    uint32_t
        vao, // Vertex array object
        quadBuffer,
        quadIndex;

private: // Setup for circles
    struct Data
    {
        glm::vec4 color;
        glm::vec2 position;
        float radius;
    };

    uint32_t counter = 0;
    uint32_t circleBuffer, numCircles;
    std::vector<Data> vData;



};