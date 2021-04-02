#pragma once

#include <glm/glm.hpp>
#include <vector>

class Quad
{

public:
    Quad(void) = default;
    ~Quad(void) = default;

    void generateObj(void);
    void draw(void);

    void cleanUp(void);

private:
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec2 texCoord;
    };

    uint32_t vao; // Vertex array object
    uint32_t vertex_buffer;
    uint32_t index_buffer;

    std::vector<uint32_t> vIndex;
    std::vector<Vertex> vtxBuffer;

}; // class-object
