#pragma once

#include <cstdint>
#include <string>
#include <vector>

using String = std::string;

class Texture
{
public:
    Texture(uint32_t width, uint32_t height);
    ~Texture(void);

    void update(const float *data);

    void bind(uint32_t slot);

    inline uint32_t getWidth(uint32_t id) { return width; }
    inline uint32_t getHeight(uint32_t id) { return height; }
    inline uint32_t getBufferID(uint32_t id) { return texID; }

private:
    uint32_t width, height, texID;
};