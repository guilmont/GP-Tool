#pragma once

#include <string>
#include <unordered_map>

class Shader
{
protected:
    uint32_t program_used;
    std::unordered_map<std::string, uint32_t> vProgram;

public:
    Shader(void);
    ~Shader(void);

    void useProgram(const std::string &name);

    void setInteger(const std::string &, int);
    void setFloat(const std::string &, float);
    void setVec2f(const std::string &, const float *);
    void setVec3f(const std::string &, const float *);
    void setVec4f(const std::string &, const float *);
    void setMatrix3f(const std::string &, const float *);
    void setMatrix4f(const std::string &, const float *);

    void setIntArray(const std::string &, const int *, int);

    void setFloatArray(const std::string &, const float *, int);
    void setVec2fArray(const std::string &, const float *, int);
    void setVec3fArray(const std::string &, const float *, int);
    void setMat3Array(const std::string &, const float *, int);
}; // class-shadear
