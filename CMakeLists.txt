cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Setup runtime output
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)


# We are always build the methods project
add_subdirectory(Methods)

option(GP_TOOL "Build graphical interface" ON)
option(BUILD_TESTS "Build test programs for trajectory and GP-FBM methods" OFF)


if (GP_TOOL)
	add_subdirectory(GP-Tool)
endif()


if (BUILD_TESTS)
	add_subdirectory(Tests)
endif()





