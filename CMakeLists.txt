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

option(GP_TOOL "Build graphical interface" OFF)

option(BUILD_TESTS "Build test programs for trajectory and GP-FBM methods" ON)
if (BUILD_TESTS)
	add_subdirectory(Tests)
endif()

# if (WIN32)
#     add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # sprintf and strcpy warnings
# endif()

#option(HIDPI "Setup scale for High DPI monitors" OFF)
#if (HIDPI)
#    add_compile_definitions(DPI_FACTOR=2)
#else()
#    add_compile_definitions(DPI_FACTOR=1)
#endif()
#
#option(COMPILE_GUI "Build graphical user interface" ON)
#option(COMPILE_LIBRARIES "Build shared libraries for batching with C++" ON)





