#pragma once

#include "gpch.h"

struct Plugin
{
    Plugin(void) = default;
    virtual ~Plugin(void) = default;

    virtual void update(float deltaTime) = 0;
    virtual void showProperties(void) = 0;
    virtual bool isActive(void) { return false;  }
    virtual void showWindows(void) {}

    virtual bool saveJSON(Json::Value &json) { return false; }
};

///////////////////////////////////////////////////////////
// Some helper functions for JSON
template <typename T>
Json::Value jsonArray(T *vec, uint64_t N)
{
    Json::Value val(Json::arrayValue);
    for (uint64_t k = 0; k < N; k++)
        val.append(vec[k]);

    return val;
}
