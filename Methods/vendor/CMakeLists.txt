cmake_minimum_required(VERSION 3.10)

###########################################################
## GLM ####################################################

add_library(glm_ INTERFACE)
target_include_directories(glm_ INTERFACE "glm")

install(DIRECTORY "glm/glm" DESTINATION "include")

###########################################################
## EIGEN ##################################################

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE "eigen")

install(FILES "eigen/Eigen/Core" "eigen/Eigen/Cholesky" "eigen/Eigen/LU" "eigen/Eigen/Jacobi"
        DESTINATION "include/Eigen")


install(DIRECTORY "eigen/Eigen/src/Core" DESTINATION "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/Cholesky" DESTINATION "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/LU" DESTINATION "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/Jacobi" DESTINATION "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/misc" DESTINATION "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/plugins" DESTINATION "include/Eigen/src")

###########################################################
## PUGIXML ################################################


add_library(pugixml STATIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/pugixml.cpp")
target_include_directories(pugixml PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(FILES "pugixml/pugiconfig.hpp" "pugixml/pugixml.hpp" 
        DESTINATION "include/pugixml")