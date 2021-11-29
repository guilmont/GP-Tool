cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

project(GPTests)
add_executable(GPTests gptests.cpp)
target_link_libraries(GPTests PUBLIC gtest_main GPMethods)
target_compile_definitions(GPTests PRIVATE GTEST_PATH="${PROJECT_SOURCE_DIR}")

include(GoogleTest)
gtest_discover_tests(GPTests)