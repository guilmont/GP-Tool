cmake_minimum_required(VERSION 3.10.0)

# Including vendors
add_subdirectory(vendor)

project(GP-Tool)


add_executable(GP-Tool "src/gptool.cpp" "include/gptool.h")
target_include_directories(GP-Tool PRIVATE include)
target_link_libraries(GP-Tool PRIVATE GRender)