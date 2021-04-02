#include "quad.h"
#include "gl_assert.cpp"

Quad::Quad(void)
{

    vIndex = {0, 1, 2, 0, 2, 3};

    vtxBuffer = {
        {{-0.5f, -0.5f, 0.0}, {0.0f, 0.0f}},
        {{+0.5f, -0.5f, 0.0}, {1.0f, 0.0f}},
        {{+0.5f, +0.5f, 0.0}, {1.0f, 1.0f}},
        {{-0.5f, +0.5f, 0.0}, {0.0f, 1.0f}},
    };

    gl_call(glad_glGenVertexArrays(1, &vao));
    gl_call(glad_glBindVertexArray(vao));

    // Copying buffer to gpu
    gl_call(glad_glGenBuffers(1, &vertex_buffer));
    gl_call(glad_glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer));
    gl_call(glad_glBufferData(GL_ARRAY_BUFFER, vtxBuffer.size() * sizeof(Vertex),
                              vtxBuffer.data(), GL_DYNAMIC_DRAW));

    // layout for buffer
    gl_call(glad_glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                                       sizeof(Vertex), nullptr));
    gl_call(glad_glEnableVertexAttribArray(0));

    gl_call(glad_glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                                       (void *)(3 * sizeof(float))));
    gl_call(glad_glEnableVertexAttribArray(1));

    // submiting index array
    gl_call(glad_glGenBuffers(1, &index_buffer));
    gl_call(glad_glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer));
    gl_call(glad_glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndex.size() * sizeof(uint32_t),
                              vIndex.data(), GL_STATIC_DRAW));

} // constructor

Quad::~Quad(void)
{
    gl_call(glad_glDeleteBuffers(1, &index_buffer));
    gl_call(glad_glDeleteBuffers(1, &vertex_buffer));
    gl_call(glad_glDeleteVertexArrays(1, &vao));
} // destructor

void Quad::draw(void)
{
    gl_call(glad_glBindVertexArray(vao));
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    gl_call(glad_glDrawElements(GL_TRIANGLES, vIndex.size(), GL_UNSIGNED_INT, 0));
    gl_call(glad_glBindVertexArray(0));
} // drawObject
