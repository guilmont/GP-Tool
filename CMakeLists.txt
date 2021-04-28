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

add_library(goptimize
    STATIC
    "${CMAKE_SOURCE_DIR}/src/utils/goptimize.cpp"
)

target_include_directories(goptimize
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/vendor"
)

#############################

add_library(mailbox
    STATIC
        "${CMAKE_SOURCE_DIR}/src/utils/mailbox.cpp"
)

target_include_directories(mailbox
    PUBLIC
       "${CMAKE_SOURCE_DIR}/include" 
       "${CMAKE_SOURCE_DIR}/vendor" 
)

target_link_libraries(mailbox
    PUBLIC
        imgui
)


###########################################################
## METHODS ################################################


set(SRC "${CMAKE_SOURCE_DIR}/src/methods/movie.cpp"
        "${CMAKE_SOURCE_DIR}/src/methods/metadata.cpp"
        "${CMAKE_SOURCE_DIR}/src/utils/gtiffer.cpp")

set(HEAD "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/include/methods"
         "${CMAKE_SOURCE_DIR}/vendor/glm"
         "${CMAKE_SOURCE_DIR}/vendor/eigen"
         "${CMAKE_SOURCE_DIR}/vendor")



add_library(movie_static STATIC ${SRC})
target_include_directories(movie_static PUBLIC ${HEAD})
target_link_libraries(movie_static PUBLIC pugixml mailbox)
target_compile_definitions(movie_static PRIVATE STATIC_API)

add_library(movie SHARED ${SRC})
target_include_directories(movie PUBLIC ${HEAD})
target_link_libraries(movie PUBLIC pugixml)


#############################

set(SRC "${CMAKE_SOURCE_DIR}/src/methods/align.cpp")

set(HEAD "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/vendor/glm"
         "${CMAKE_SOURCE_DIR}/vendor/eigen"
         "${CMAKE_SOURCE_DIR}/include/methods")

add_library(align_static STATIC ${SRC})
target_include_directories(align_static PUBLIC ${HEAD})
target_link_libraries(align_static PUBLIC goptimize mailbox)
target_compile_definitions(align_static PRIVATE STATIC_API)

add_library(align SHARED ${SRC})
target_include_directories(align PUBLIC ${HEAD})
target_link_libraries(align PUBLIC goptimize)
         

#############################

set(SRC "${CMAKE_SOURCE_DIR}/src/methods/spot.cpp"
        "${CMAKE_SOURCE_DIR}/src/methods/trajectory.cpp")

set(HEAD "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/vendor/glm"
         "${CMAKE_SOURCE_DIR}/vendor/eigen"
         "${CMAKE_SOURCE_DIR}/include/methods")


add_library(trajectory_static STATIC ${SRC})
target_include_directories(trajectory_static PUBLIC ${HEAD})
target_link_libraries(trajectory_static PUBLIC pugixml goptimize mailbox)
target_compile_definitions(trajectory_static PRIVATE STATIC_API)

add_library(trajectory SHARED ${SRC})
target_include_directories(trajectory PUBLIC ${HEAD})
target_link_libraries(trajectory PUBLIC pugixml goptimize movie)

#############################

set(SRC "${CMAKE_SOURCE_DIR}/src/methods/gp_fbm.cpp")

set(HEAD "${CMAKE_SOURCE_DIR}/include"
         "${CMAKE_SOURCE_DIR}/vendor/glm"
         "${CMAKE_SOURCE_DIR}/vendor/eigen"
         "${CMAKE_SOURCE_DIR}/include/methods")


add_library(gpfbm_static STATIC ${SRC})
target_include_directories(gpfbm_static PUBLIC ${HEAD})
target_link_libraries(gpfbm_static PUBLIC goptimize mailbox)
target_compile_definitions(gpfbm_static PRIVATE STATIC_API)

add_library(gpfbm SHARED ${SRC})
target_include_directories(gpfbm PUBLIC ${HEAD})
target_link_libraries(gpfbm PUBLIC goptimize mailbox)

###########################################################
## RENDERER ###############################################

add_library(renderer 
    STATIC
        "${CMAKE_SOURCE_DIR}/src/renderer/framebuffer.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/shader.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/texture.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/quad.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/camera.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/fonts.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/gdialog.cpp"
        "${CMAKE_SOURCE_DIR}/src/renderer/renderer.cpp"
)

target_include_directories(renderer
    PUBLIC  
        "${CMAKE_SOURCE_DIR}/vendor"
        "${CMAKE_SOURCE_DIR}/vendor/glm"
        "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries(renderer
    PUBLIC
        implot
        mailbox
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
    "${CMAKE_SOURCE_DIR}/src/plugins/gpPlugin.cpp"
)

target_include_directories(GPTool
    PUBLIC
        "${CMAKE_SOURCE_DIR}/include"
        "${CMAKE_SOURCE_DIR}/include/plugins"
)

target_link_libraries(GPTool
    PUBLIC
        Threads::Threads
        renderer
        movie_static
        align_static
        trajectory_static
        gpfbm_static
        jsoncpp
)


###########################################################
## INSTALLATION SETUP #####################################



install(
    TARGETS 
        movie
        align
        trajectory
        gpfbm

    DESTINATION lib
)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/include/methods/"
    DESTINATION include
    FILES_MATCHING PATTERN "*.h*"
)

install(
    FILES 
        "${CMAKE_SOURCE_DIR}/include/utils/gtiffer.h"
        "${CMAKE_SOURCE_DIR}/include/utils/goptimize.h"
    DESTINATION 
        include/utils
)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/glm/glm"
    DESTINATION include
    FILES_MATCHING 
        PATTERN "*.h*"
        PATTERN "*.inl"
)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/eigen/Eigen"
    DESTINATION include
)

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/vendor/pugixml"
    DESTINATION include
     FILES_MATCHING PATTERN "*.h*"
)

install(
    DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
    DESTINATION assets
    FILES_MATCHING PATTERN "*"
)


install(
    TARGETS GPTool
    DESTINATION bin
)


