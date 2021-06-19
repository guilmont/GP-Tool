cmake_minimum_required(VERSION 3.10.0)

# Including vendors
add_subdirectory(vendor)

project(GP-Tool)

set(SHADER_PATH ${PROJECT_SOURCE_DIR}/assets/shaders)

add_executable(GP-Tool
	"include/gpch.h"
	"include/gptool.h"      "src/gptool.cpp"
	"include/plugin.h" 
	"include/mailbox.h"      "src/mailbox.cpp" 
	"include/moviePlugin.h"  "src/moviePlugin.cpp" 
	"include/alignPlugin.h"  "src/alignPlugin.cpp" 
	"include/trajPlugin.h"   "src/trajPlugin.cpp" 
	"include/gpPlugin.h"     "src/gpPlugin.cpp"
	"src/app.cpp"
)

target_include_directories(GP-Tool PRIVATE include)
target_precompile_headers(GP-Tool PRIVATE "include/gpch.h")

target_link_libraries(GP-Tool PUBLIC GRender jsoncpp Methods)
target_compile_definitions(GP-Tool PRIVATE SHADER_PATH="${SHADER_PATH}")
