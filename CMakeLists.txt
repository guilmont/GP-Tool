cmake_minimum_required(VERSION 3.10.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Setup runtime output
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR} CACHE PATH "Location to install this application")

if (NOT WIN32)
	set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX})
endif()

project(GP_FBM_LIBRARY VERSION 0.2.0)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # Removing sprintf warnings

option(GP_TOOL "Build graphical interface" ON)
#option(GP_PYTHON "Build python bindings for GP-FBM methods" ON)
option(GP_PRECOMPILED_HEADERS "Builds precompiled headers for whole project" OFF)

# We are always build the methods project
add_subdirectory(Methods)


if (GP_TOOL)
	add_subdirectory(GP-Tool)
endif()
