#pragma once

#include <iostream>
#include <fstream>
#include <thread>
#include <string>
#include <memory>
#include <functional>

#include <chrono>
#include <random>
#include <numeric>

#include <list>
#include <vector>

#include <unordered_map>

#include <GRender.h>
#include <imgui.h>

#include <json/json.h>


static bool SliderU64(const char* label, uint64_t* data, uint64_t min, uint64_t max)
{
    return ImGui::SliderScalar(label, ImGuiDataType_U64, data, &min, &max, "%lld");
}