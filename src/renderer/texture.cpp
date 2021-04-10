#include "texture.h"

#include "gl_assert.cpp"

Texture::Texture(uint32_t width, uint32_t height) : width(width), height(height)
{
    gl_call(glad_glGenTextures(1, &texID));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, texID));

    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    gl_call(glad_glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    // I'll pass a black texture
    gl_call(glad_glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0,
                              GL_RED, GL_FLOAT, nullptr));

} // constructor

Texture::~Texture(void)
{
    gl_call(glad_glDeleteTextures(1, &texID));
} // destructor

void Texture::bind(uint32_t slot)
{
    gl_call(glad_glActiveTexture(GL_TEXTURE0 + slot));
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, texID));

} // bind

void Texture::update(const float *data)
{
    gl_call(glad_glBindTexture(GL_TEXTURE_2D, texID));
    gl_call(glad_glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data));
} // updateData
