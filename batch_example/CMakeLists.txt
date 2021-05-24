cmake_minimum_required(VERSION 3.0.0)
project(batch VERSION 0.1.0)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)
find_package(Threads REQUIRED)

if (WIN32)
    set(GP_INCLUDE "C:/Users/GM Oliveira/Desktop/GP-Tool/include")
    set(GP_LIB "C:/Users/GM Oliveira/Desktop/GP-Tool/lib")

else()
    set(GP_INCLUDE "/home/gstark/GP/include")
    set(GP_LIB "/home/gstark/GP/lib")
endif()


add_executable(batch main.cpp)

target_include_directories(batch PUBLIC ${GP_INCLUDE})
target_link_directories(batch PUBLIC ${GP_LIB})

target_link_libraries(batch
    PUBLIC  
        Threads::Threads
        movie
        align
        trajectory
        gpfbm
)
