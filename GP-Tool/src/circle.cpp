#include "circle.h"

Circle::Circle(uint32_t numCircles, uint32_t numVertices) : maxVertices(numVertices * numCircles)
{

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Copying buffer to gpu
    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, maxVertices * sizeof(Vertex), nullptr, GL_DYNAMIC_DRAW);

    // layout for buffer
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);

    // Preparing index array
    uint32_t ct = 0;
    vIndex.resize(2*numVertices * numCircles);
    for (uint32_t k = 0; k < numCircles; k++)
    {
        uint32_t loc = ct >> 1;
        for (uint32_t l = 0; l < numVertices-1; l++)
        {
            vIndex[ct++] = loc + l;
            vIndex[ct++] = loc + l + 1;
        }

        vIndex[ct++] = loc+numVertices-1;
        vIndex[ct++] = loc;

    }

    glGenBuffers(1, &index_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndex.size() * sizeof(uint32_t), vIndex.data(), GL_STATIC_DRAW);

    // Allocating memory for vertex buffer
    vtxBuffer.resize(maxVertices);

    // Generating unitary circle as template for later
    float dAng = 2.0f * float(M_PI) / float(numVertices);

    unitary.resize(numVertices);
    for (uint32_t k = 0; k < numVertices; k++)
    {
        float angle = float(k) * dAng;
        unitary[k] = { cos(angle), sin(angle) };
    }

}

Circle::~Circle(void)
{
    if (vao> 0)
    {
        glDeleteBuffers(1, &index_buffer);
        glDeleteBuffers(1, &vertex_buffer);
        glDeleteVertexArrays(1, &vao);
        
        index_buffer = vertex_buffer = vao = 0;
    }
}

void Circle::setThickness(float val)
{
    assert(val > 0.0f);  // Otherwise it doesn't make sense
    thickness = val;
}

void Circle::draw(const glm::vec3 &position, float radius, const glm::vec4 &color)
{
    assert(counter < maxVertices); // To make sure that we didn't exceed the maximum size

    for (glm::vec2& var : unitary)
        vtxBuffer[counter++] = {{radius * var.x + position.x, radius * var.y + position.y, 0.0f},color};
}

void Circle::submit(void)
{
    // Binding buffers
    glBindVertexArray(vao);
    glad_glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

    // Let's send our data to the GPU
    glBufferSubData(GL_ARRAY_BUFFER, 0, counter * sizeof(Vertex), vtxBuffer.data());

    // Issueing the draw call
    glad_glLineWidth(thickness);
    glDrawElements(GL_LINES, 2 * counter, GL_UNSIGNED_INT, 0);

    // Resetting counter to zero for next round
    counter = 0;

} // drawObject

