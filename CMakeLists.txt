cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)

# We need standard 17 for some functions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)


# GENERAL PURPOSE VENDOR
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor")

# CONFIGURATION FILE
set(HOME_DIR "$ENV{HOME}")
set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(INI_PATH "${CMAKE_SOURCE_DIR}/assets/gptool.ini")
configure_file("config.h.in" "${CMAKE_SOURCE_DIR}/include/renderer/config.h" @ONLY)

##################################################################
##################################################################
# GRAPHICAL USER INTERFACE

add_library(RENDERER 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/renderer/shader.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/texture.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/framebuffer.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/quad.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/camera.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/fonts.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/gdialog.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/mailbox.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/renderer.cpp"
)

target_include_directories(RENDERER
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/include/renderer"
        "${CMAKE_SOURCE_DIR}/vendor/stb"
        "${GLM_INCLUDE_DIRS}"
)

target_link_libraries(RENDERER
    PUBLIC
        glfw
        glad
        imgui
)


#########################

add_executable(GPTool 
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/gptool.cpp"
)

set_target_properties(GPTool
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(GPTool
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        # "${HDF5_CXX_INCLUDE_DIRS}"
)

target_link_libraries(GPTool
    PUBLIC
        RENDERER
        # PkgConfig::jsoncpp
        # "${HDF5_CXX_LIBRARIES}"
)



# install(TARGETS GPTool DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/gui/assets" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(FILES "${CMAKE_SOURCE_DIR}/gui/gptool.ini" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/GPTool.sh" DESTINATION "${CMAKE_INSTALL_PREFIX}")



