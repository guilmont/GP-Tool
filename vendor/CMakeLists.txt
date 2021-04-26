cmake_minimum_required(VERSION 3.10)

# find_package(PkgConfig REQUIRED)
# find_package(HDF5 COMPONENTS CXX REQUIRED) 
# pkg_check_modules(jsoncpp REQUIRED IMPORTED_TARGET jsoncpp)



###########################################################
## GLFW ###################################################
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glfw" glfw)

###########################################################
## GLM ####################################################

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/glm" glm)


###########################################################
## GLAD ###################################################

add_library(glad 
    STATIC
        "${CMAKE_CURRENT_SOURCE_DIR}/glad/src/glad.c"
)   

target_link_libraries(glad 
    PUBLIC 
        "${CMAKE_DL_LIBS}"
)

target_include_directories(glad
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/glad/include"
)

# set_property(TARGET glad PROPERTY POSITION_INDEPENDENT_CODE ON)

###########################################################
## PUGIXML ################################################

add_library(pugixml
    STATIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src/pugixml.cpp"
)

target_include_directories(pugixml
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/src"
)


###########################################################
## IMGUI ##################################################

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)

add_library(imgui 
    STATIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_draw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_widgets.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_tables.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/imgui_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_glfw.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

target_link_libraries(imgui 
    PUBLIC 
        glad 
        glfw
)

target_include_directories(imgui
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui"
        "${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends"
)

###########################################################
## IMPLOT #################################################

add_library(implot 
    STATIC 
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_demo.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/implot/implot_items.cpp"
)

target_include_directories(implot
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/implot"
)

target_link_libraries(implot
    PUBLIC
        imgui
)
