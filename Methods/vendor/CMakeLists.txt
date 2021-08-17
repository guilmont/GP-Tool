cmake_minimum_required(VERSION 3.10)

###########################################################
## EIGEN ##################################################

add_library(eigen INTERFACE)
target_include_directories(eigen INTERFACE 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/eigen>
        $<INSTALL_INTERFACE:${GP_FBM_INC}>
)

install(FILES "eigen/Eigen/Core" "eigen/Eigen/Cholesky" "eigen/Eigen/LU" "eigen/Eigen/Jacobi"
        DESTINATION "${GP_FBM_INC}/Eigen")

set(EIGEN_DEST "${GP_FBM_INC}/Eigen/src")
install(DIRECTORY "eigen/Eigen/src/Core" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/Cholesky" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/LU" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/Jacobi" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/misc" DESTINATION ${EIGEN_DEST})
install(DIRECTORY "eigen/Eigen/src/plugins" DESTINATION ${EIGEN_DEST})



###########################################################
## PUGIXML ################################################

add_library(pugixml STATIC "${CMAKE_CURRENT_SOURCE_DIR}/pugixml/pugixml.cpp")
target_include_directories(pugixml PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${GP_FBM_INC}>        
)
set_target_properties(pugixml PROPERTIES POSITION_INDEPENDENT_CODE ON)

install(FILES "pugixml/pugiconfig.hpp" "pugixml/pugixml.hpp" 
        DESTINATION "${GP_FBM_INC}/pugixml")

###########################################################

install(TARGETS eigen pugixml EXPORT ${GP_EXPORT})