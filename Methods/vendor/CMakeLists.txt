cmake_minimum_required(VERSION 3.10)

###########################################################
## GLM ####################################################

add_library(glm_ INTERFACE)
target_include_directories(glm_ INTERFACE "glm")


###########################################################
## EIGEN ##################################################

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE "eigen")


###########################################################
## PUGIXML ################################################


add_library(pugixml STATIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/pugixml.cpp")
target_include_directories(pugixml PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml")
set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)
