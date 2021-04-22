cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)

# We need standard 17 for some functions
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)



# GENERAL PURPOSE VENDOR
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor")

# CONFIGURATION FILE
option(HIGH_DPI "Setup scale for High DPI monitors" OFF)

if (${HIGH_DPI})
    set(DPI_FACTOR "2")
else()
    set(DPI_FACTOR "1")
endif()

if (WIN32)
    file(TO_CMAKE_PATH "$ENV{USERPROFILE}" HOME_DIR)
else()
    set(HOME_DIR "$ENV{HOME}")
endif()


set(PROJECT_DIR "${CMAKE_SOURCE_DIR}")
set(INI_PATH "${CMAKE_SOURCE_DIR}/assets/gptool.ini")
configure_file("${CMAKE_SOURCE_DIR}/config.h.in" "${CMAKE_SOURCE_DIR}/include/renderer/config.h" NEWLINE_STYLE UNIX)


###########################################################
# UTILS

add_library(GOPTIMIZE
    STATIC
    "${CMAKE_SOURCE_DIR}/src/utils/goptimize.cpp"
)

target_include_directories(GOPTIMIZE
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/vendor"
)

#############################

add_library(MAILBOX
    STATIC
        "${CMAKE_SOURCE_DIR}/src/utils/mailbox.cpp"
)

target_include_directories(MAILBOX
    PUBLIC
       "${CMAKE_SOURCE_DIR}/include" 
       "${CMAKE_SOURCE_DIR}/vendor" 
)

target_link_libraries(MAILBOX
    PUBLIC
        imgui
)

###########################################################
# METHODS


add_library(MOVIE
    STATIC
        "${CMAKE_SOURCE_DIR}/src/methods/movie.cpp"
        "${CMAKE_SOURCE_DIR}/src/methods/metadata.cpp"
        "${CMAKE_SOURCE_DIR}/src/utils/gtiffer.cpp"
)

target_include_directories(MOVIE 
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/vendor"
    "${glm_INCLUDE_DIRS}"
)

target_link_libraries(MOVIE
    PUBLIC
        pugixml
        MAILBOX
        glm
)

#############################

add_library(ALIGN
    STATIC
        "${CMAKE_SOURCE_DIR}/src/methods/align.cpp"
)

target_include_directories(ALIGN 
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/methods"
)

target_link_libraries(ALIGN
    PUBLIC
        imgui
        GOPTIMIZE
        MOVIE
)


#############################

add_library(TRAJECTORY
    STATIC
        "${CMAKE_SOURCE_DIR}/src/methods/spot.cpp"
        "${CMAKE_SOURCE_DIR}/src/methods/trajectory.cpp"

)

target_include_directories(TRAJECTORY 
    PUBLIC
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/methods"
)

target_link_libraries(TRAJECTORY
    PUBLIC
        pugixml
        GOPTIMIZE
        MOVIE
)


###########################################################
# RENDERER FOR GUI

add_library(RENDERER 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/renderer/shader.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/texture.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/framebuffer.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/quad.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/camera.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/fonts.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/gdialog.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/renderer.cpp"
)

target_include_directories(RENDERER
    PUBLIC 
        "${CMAKE_SOURCE_DIR}/vendor"
        "${CMAKE_SOURCE_DIR}/include"
        "${glm_INCLUDE_DIRS}"
)

target_link_libraries(RENDERER
    PUBLIC
        glfw
        glad
        imgui
        MAILBOX
        glm
)

###########################################################
# PLUGIN MANAGER AND MAIN EXECUTABLE

add_executable(GPTOOL
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/gptool.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/moviePlugin.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/alignPlugin.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/trajPlugin.cpp"
)

target_include_directories(GPTOOL
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/plugins"
)

target_link_libraries(GPTOOL
    PUBLIC
        RENDERER
        MOVIE
        ALIGN
        TRAJECTORY
        Threads::Threads
)


###########################################################
# INSTALLATION SETUP

# install(TARGETS GPTool DESTINATION "${CMAKE_INSTALL_PREFIX}/bin")
# install(DIRECTORY "${CMAKE_SOURCE_DIR}/gui/assets" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(FILES "${CMAKE_SOURCE_DIR}/gui/gptool.ini" DESTINATION "${CMAKE_INSTALL_PREFIX}")
# install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/GPTool.sh" DESTINATION "${CMAKE_INSTALL_PREFIX}")



