#pragma once

#include <glm/glm.hpp>

class Framebuffer
{
public:
    Framebuffer(uint32_t width, uint32_t height);
    ~Framebuffer(void);

    void bind(void);
    void unbind(void);

    inline glm::ivec2 getDimensions(void) const { return dimension; }
    inline uint32_t getID(void) { return textureID; }

private:
    glm::ivec2 dimension;
    uint32_t bufferID = 0, textureID = 0;
};