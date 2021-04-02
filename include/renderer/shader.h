#pragma once

#include <map>
#include <string>
#include <cstring>

using String = std::string;

class Shader
{
protected:
    uint32_t program_used;
    std::map<String, uint32_t> vProgram;

public:
    Shader(void) = default;
    ~Shader(void) = default;

    void loadShaders(void);
    void cleanUp(void);

    void useProgram(const String &name);

    void setInteger(const String &, int);
    void setFloat(const String &, float);
    void setVec2f(const String &, const float *);
    void setVec3f(const String &, const float *);
    void setVec4f(const String &, const float *);
    void setMatrix3f(const String &, const float *);
    void setMatrix4f(const String &, const float *);

    void setFloatArray(const String &, const float *, size_t);
    void setVec2fArray(const String &, const float *, size_t);
}; // class-shadear
