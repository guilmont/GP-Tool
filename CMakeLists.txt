cmake_minimum_required(VERSION 3.10.0)
project(GP_FBM_LIBRARY VERSION 0.2.0)

# CONFIGURATION 

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package(Threads REQUIRED)

if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)  # sprintf and strcpy warnings
endif()

option(HIGH_DPI "Setup scale for High DPI monitors" OFF)
if (HIGH_DPI)
    add_compile_definitions(DPI_FACTOR=2)
else()
    add_compile_definitions(DPI_FACTOR=1)
endif()


###########################################################
## VENDOR #################################################
add_subdirectory("${CMAKE_SOURCE_DIR}/vendor")


###########################################################
## UTILS ##################################################

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
## METHODS ################################################


add_library(MOVIE
    SHARED
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
    SHARED
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
    SHARED
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


#############################

add_library(GP_FBM
    SHARED
        "${CMAKE_SOURCE_DIR}/src/methods/gp_fbm.cpp"    
)

target_include_directories(GP_FBM
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/methods"
)

target_link_libraries(GP_FBM
    PUBLIC
        GOPTIMIZE
        MAILBOX
)

###########################################################
## RENDERER ###############################################

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
        implot
)

###########################################################
## EXECUTABLE #############################################

add_executable(GPTool
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
    "${CMAKE_SOURCE_DIR}/src/gptool.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/moviePlugin.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/alignPlugin.cpp"
    "${CMAKE_SOURCE_DIR}/src/plugins/trajPlugin.cpp"
)

target_include_directories(GPTool
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/plugins"
)

target_link_libraries(GPTool
    PUBLIC
        RENDERER
        MOVIE
        ALIGN
        TRAJECTORY
        Threads::Threads
        implot
)

###########################################################
## INSTALLATION SETUP #####################################


# install(
#     TARGETS 
#         MOVIE
#         ALIGN
#         TRAJECTORY

#     DESTINATION lib
# )

# install(
#     DIRECTORY ${CMAKE_SOURCE_DIR}/include/
#     DESTINATION include
#     FILES_MATCHING PATTERN "*.h*"
# )

# install(
#     DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
#     DESTINATION assets
#     FILES_MATCHING PATTERN "*"
# )


# install(
#     TARGETS GPTool
#     DESTINATION bin
# )


