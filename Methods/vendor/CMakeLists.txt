cmake_minimum_required(VERSION 3.10)

###########################################################
## EIGEN ##################################################

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/eigen>
        $<INSTALL_INTERFACE:include>
)

install(FILES "eigen/Eigen/Core" "eigen/Eigen/Cholesky" "eigen/Eigen/LU" "eigen/Eigen/Jacobi"
        DESTINATION "include/Eigen")

set(EIGEN_DEST "include/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/Core" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/Cholesky" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/LU" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/Jacobi" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/misc" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/plugins" DESTINATION ${EIGEN_DEST})

install(TARGETS eigen EXPORT ${GP_EXPORT})

###########################################################
## PUGIXML ################################################

add_library(pugixml STATIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/pugixml.cpp")
target_include_directories(pugixml PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>        
)
set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(FILES "pugixml/pugiconfig.hpp" "pugixml/pugixml.hpp" 
        DESTINATION "include/pugixml")

install(TARGETS pugixml 
        EXPORT ${GP_EXPORT}
        ARCHIVE DESTINATION "lib/static"        
)

#########################################################
## GoogleTests #########################################

if (BUILD_GOOGLETESTS)
    set(BUILD_GMOCK OFF CACHE BOOL "We want to install only GTest")
    set(INSTALL_GTEST OFF CACHE BOOL "No need to install GTest")
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)  # This is necessary to runtime shared library

    enable_testing()
    add_subdirectory("GoogleTests")
endif()

###########################################################

