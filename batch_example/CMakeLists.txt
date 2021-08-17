cmake_minimum_required(VERSION 3.0.0)
project(batch VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)

find_package(GPMethods REQUIRED)  # Used to find GP-Tool methods installation


add_executable(batch main.cpp)
target_link_libraries(batch PUBLIC GPMethods)
