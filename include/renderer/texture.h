#pragma once

#include <cstdint>
#include <string>
#include <vector>

using String = std::string;

class Texture
{
public:
    Texture(void) = default;
    ~Texture(void);

    void create(uint32_t width, uint32_t height);
    void update(uint32_t id, float *data);

    void bind(uint32_t channel, uint32_t slot);

    inline uint32_t getWidth(uint32_t id) { return vTexture.at(id).width; }
    inline uint32_t getHeight(uint32_t id) { return vTexture.at(id).height; }
    inline uint32_t getBufferID(uint32_t id) { return vTexture.at(id).id; }

private:
    struct TexInfo
    {
        uint32_t width, height, id;
    };

    std::vector<TexInfo> vTexture;
};