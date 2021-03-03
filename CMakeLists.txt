cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)

# We need standard 17 for some functions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED YES)
set(CMAKE_CXX17_STANDARD_COMPILE_OPTION "-std=c++17")
set(CMAKE_CXX17_EXTENSION_COMPILE_OPTION "-std=c++17")
set(CMAKE_BUILD_TYPE "Release")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")
set(CMAKE_THREAD_LIBS_INIT "-lpthread")


# GENERAL PURPOSE VENDOR
find_package(HDF5 COMPONENTS CXX REQUIRED) 

add_compile_definitions(MESSAGE_IMGUI)
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor")


find_package(PkgConfig REQUIRED)
pkg_check_modules(jsoncpp REQUIRED IMPORTED_TARGET jsoncpp)


###########################################################
# CREATING LIBRARIES

add_library(ALIGN 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/align.cpp"
)

target_include_directories(ALIGN
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/vendor/eigen"
)

#############################

add_library(GP_FBM 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/gp_fbm.cpp"
)

target_include_directories(GP_FBM
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/vendor/eigen"

)

#############################


add_library(META 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/metadata.cpp"
)

target_include_directories(META
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/vendor/eigen"

)

 target_link_libraries(META 
    PUBLIC 
        pugixml
)

#############################

add_library(MOVIE 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/movie.cpp"
)

target_include_directories(MOVIE
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/vendor/eigen"
)

target_link_libraries(MOVIE 
    PUBLIC 
        TIFFER
        META
)

#############################

add_library(TIFFER
    STATIC  
        "${CMAKE_SOURCE_DIR}/src/tiffer.cpp"
)

target_include_directories(TIFFER
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"        
        "${CMAKE_SOURCE_DIR}/vendor/eigen"
)

target_link_libraries(TIFFER 
    PUBLIC 
        THRPOOL
)

#############################

add_library(THRPOOL
    STATIC
        "${CMAKE_SOURCE_DIR}/src/threadpool.cpp"
)

target_include_directories(THRPOOL
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

#############################

add_library(TRAJECTORY 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/trajectory.cpp"
        "${CMAKE_SOURCE_DIR}/src/spot.cpp"
)

target_include_directories(TRAJECTORY
    PUBLIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(TRAJECTORY 
    PUBLIC 
        MOVIE
)

#############################

add_library(MAILBOX
    STATIC
        "${CMAKE_SOURCE_DIR}/src/mailbox.cpp"
)

target_include_directories(MAILBOX
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(MAILBOX
    PUBLIC
        imgui
)


##################################################################
##################################################################
# GRAPHICAL USER INTERFACE

add_library(RENDERER 
    STATIC
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/shader.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/texture.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/framebuffer.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/camera.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/quad.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/fonts.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/fileDialog.cpp"
        "${CMAKE_SOURCE_DIR}/gui/renderer/src/ui.cpp"
)

target_include_directories(RENDERER
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/gui/renderer/include"
        "${CMAKE_SOURCE_DIR}/gui/include"
        "${CMAKE_SOURCE_DIR}/vendor/stb"
)

target_link_libraries(RENDERER
    PUBLIC
        glm
        glfw
        glad
        imgui
        implot
        MAILBOX
        MOVIE
        ALIGN
        TRAJECTORY
        GP_FBM
)


#########################

add_executable(GPTool 
    "${CMAKE_SOURCE_DIR}/gui/main.cpp"
    "${CMAKE_SOURCE_DIR}/gui/ui/src/imageProperties.cpp"
    "${CMAKE_SOURCE_DIR}/gui/ui/src/childWindows.cpp"
)

set_target_properties(GPTool
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/gui"
)

target_include_directories(GPTool
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/gui/ui/include"
        "${HDF5_CXX_INCLUDE_DIRS}"
)

target_link_libraries(GPTool
    PUBLIC
        RENDERER
        PkgConfig::jsoncpp
        "${HDF5_CXX_LIBRARIES}"
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/GPTool.sh"
"#!/usr/bin/env bash
 
 cd ${CMAKE_INSTALL_PREFIX}/
./bin/GPTool 
")

install(TARGETS GPTool DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
install(DIRECTORY "${CMAKE_SOURCE_DIR}/gui/assets" DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(FILES "${CMAKE_SOURCE_DIR}/gui/gptool.ini" DESTINATION "${CMAKE_INSTALL_PREFIX}")
install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/GPTool.sh" DESTINATION "${CMAKE_INSTALL_PREFIX}")



