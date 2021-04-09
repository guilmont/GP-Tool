#pragma once

#include "config.h"

class Shader
{
protected:
    uint32_t program_used;
    std::unordered_map<String, uint32_t> vProgram;

public:
    Shader(void);
    ~Shader(void);

    void useProgram(const String &name);

    void setInteger(const String &, int);
    void setFloat(const String &, float);
    void setVec2f(const String &, const float *);
    void setVec3f(const String &, const float *);
    void setVec4f(const String &, const float *);
    void setMatrix3f(const String &, const float *);
    void setMatrix4f(const String &, const float *);

    void setIntArray(const String &, const int *, size_t);

    void setFloatArray(const String &, const float *, size_t);
    void setVec2fArray(const String &, const float *, size_t);
    void setVec3fArray(const String &, const float *, size_t);
}; // class-shadear
