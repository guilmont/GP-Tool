#include "circle.h"

Circle::Circle(uint32_t numCircles) : numCircles(numCircles)
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    /****************************************************************/
    // Let's setup the quad first

    // Preparing index array
    uint32_t vIndex[] = {0, 1, 2, 0, 2, 3};
    glGenBuffers(1, &quadIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(vIndex), vIndex, GL_STATIC_DRAW);

    // Setting up the vertices
    std::array<QuadVertex, 4> vBuffer;
    vBuffer[0] = {{-0.5f, -0.5f, 0.00f}, {0.0f, 0.0f}};
    vBuffer[1] = {{+0.5f, -0.5f, 0.00f}, {1.0f, 0.0f}};
    vBuffer[2] = {{+0.5f, +0.5f, 0.00f}, {1.0f, 1.0f}};
    vBuffer[3] = {{-0.5f, +0.5f, 0.00f}, {0.0f, 1.0f}};

    glGenBuffers(1, &quadBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(QuadVertex), vBuffer.data(), GL_DYNAMIC_DRAW);

    // layout for buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void *)offsetof(QuadVertex, position));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(QuadVertex), (const void *)offsetof(QuadVertex, texCoord));
    glEnableVertexAttribArray(1);

    /****************************************************************/
    // Now we prepare the terrain for the circles

    vData.resize(numCircles);

    glGenBuffers(1, &circleBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, circleBuffer);
    glBufferData(GL_ARRAY_BUFFER, numCircles * sizeof(Data), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Data), (const void *)offsetof(Data, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (const void *)offsetof(Data, position));
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Data), (const void *)offsetof(Data, radius));
    glEnableVertexAttribArray(4);

    // Setup step for instancing
    glVertexAttribDivisor(2, 1);
    glVertexAttribDivisor(3, 1);
    glVertexAttribDivisor(4, 1);

}

Circle::~Circle(void)
{
    if (vao > 0)
    {
        glDeleteBuffers(1, &quadIndex);
        glDeleteBuffers(1, &quadBuffer);
        glDeleteBuffers(1, &circleBuffer);
        glDeleteVertexArrays(1, &vao);
        
        quadIndex = quadBuffer = circleBuffer = 0;
    }
}

void Circle::draw(const glm::vec2 &position, float radius, const glm::vec4 &color)
{
    assert(counter < numCircles); 
    vData[counter++] = {color, position, radius};
}


void Circle::submit(void)
{
    glBindVertexArray(vao);

    // Updating circle positions
    glBindBuffer(GL_ARRAY_BUFFER, circleBuffer);
    glBufferSubData(GL_ARRAY_BUFFER, 0, counter * sizeof(Data), vData.data());

    // Binding quad
    glBindBuffer(GL_ARRAY_BUFFER, quadBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIndex);

    // Issueing the draw call
    glDrawElementsInstanced(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr, int32_t(counter));

    // Resetting
    glBindVertexArray(0);
    counter = 0;
}