#include "texture.h"

#include "gl_assert.cpp"

Texture::~Texture(void)
{
    for (TexInfo tex : vTexture)
    {
        gl_call(glad_glDeleteTextures(1, &tex.id));
    }

} // destructor

void Texture::bind(uint32_t ch, uint32_t slot)
{
    gl_call(glad_glActiveTexture(GL_TEXTURE0 + slot));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, vTexture.at(ch).id));

} // bind

void Texture::create(uint32_t width, uint32_t height)
{
    TexInfo tex = {width, height, 0};

    gl_call(glad_glGenTextures(1, &tex.id));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // I'll pass a black texture
    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8,
                              tex.width, tex.height, 0, GL_RED, GL_FLOAT, nullptr));

    vTexture.push_back(tex);

} // constructor

void Texture::update(uint32_t id, float *data)
{
    TexInfo &tex = vTexture.at(id);

    gl_call(glad_glBindTexture(GL_TEXTURE_2D, tex.id));
    gl_call(glad_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, tex.width, tex.height,
                                 GL_RED, GL_FLOAT, data));

} // updateData
