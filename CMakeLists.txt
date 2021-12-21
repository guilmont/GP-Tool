cmake_minimum_required(VERSION 3.10.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Setup runtime output
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin)

project(GP_FBM_LIBRARY VERSION 0.4.0)

option(GP_TOOL "Build graphical interface" ON)
option(GP_PRECOMPILED_HEADERS "Builds precompiled headers for whole project" OFF)
option(BUILD_GOOGLETESTS "Builds GPMethods tests interface using GoogleTests (won't be installed)" OFF)

# We always build the methods project
add_subdirectory(Methods)

if (GP_TOOL)
	add_subdirectory(GP-Tool)
endif()


## Setup installer package
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/License.txt")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "4")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/packages")
include(CPack)