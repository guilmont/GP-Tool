cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

project(GPTests)

add_executable(testAlign testAlign.cpp)
target_link_libraries(testAlign PUBLIC gtest_main GPMethods jsoncpp)
target_compile_definitions(testAlign PRIVATE GTEST_PATH="${PROJECT_SOURCE_DIR}")

add_executable(testGP testGP.cpp)
target_link_libraries(testGP PUBLIC gtest_main GPMethods jsoncpp)
target_compile_definitions(testGP PRIVATE GTEST_PATH="${PROJECT_SOURCE_DIR}")


include(GoogleTest)
gtest_discover_tests(testAlign testGP)