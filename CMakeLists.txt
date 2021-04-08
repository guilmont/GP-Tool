cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)

# We need standard 17 for some functions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# GENERAL PURPOSE VENDOR
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor")

# CONFIGURATION FILE
set(DPI_FACTOR "1")
set(HOME_DIR "$ENV{HOME}")
set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(INI_PATH "${CMAKE_SOURCE_DIR}/assets/gptool.ini")
configure_file("config.h.in" "${CMAKE_SOURCE_DIR}/include/renderer/config.h" @ONLY)

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

###########################################################
# METHODS
add_library(MOVIE
    SHARED
        "${CMAKE_SOURCE_DIR}/src/methods/movie.cpp"
        "${CMAKE_SOURCE_DIR}/src/methods/metadata.cpp"
        "${CMAKE_SOURCE_DIR}/src/utils/gtiffer.cpp"
)

target_include_directories(MOVIE 
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/methods"
    "${CMAKE_SOURCE_DIR}/vendor/eigen"
)

target_link_libraries(MOVIE
    PUBLIC
        imgui
        pugixml
)


###########################################################
# PLUGIN MANAGER ##########################################

add_library(PMANAGER
    STATIC
        "${CMAKE_SOURCE_DIR}/src/plugins/manager.cpp"
        "${CMAKE_SOURCE_DIR}/src/plugins/moviePlugin.cpp"
)

target_include_directories(PMANAGER
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/plugins"
)

target_link_libraries(PMANAGER
    PUBLIC
        MOVIE
)


###########################################################
# MAIN EXECUTABLE

add_executable(GPTool 
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/gptool.cpp"
)


target_include_directories(GPTool
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(GPTool
    PUBLIC
        RENDERER
        PMANAGER
)



# install(TARGETS GPTool DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/gui/assets" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(FILES "${CMAKE_SOURCE_DIR}/gui/gptool.ini" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/GPTool.sh" DESTINATION "${CMAKE_INSTALL_PREFIX}")



