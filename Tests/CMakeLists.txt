cmake_minimum_required(VERSION 3.10.0)
project(Tests)


add_executable(Traj trajectories/main.cpp)
target_link_libraries(Traj PUBLIC Methods)
target_compile_definitions(Traj PRIVATE OPATH="${PROJECT_SOURCE_DIR}/trajectories")

#########################################

add_executable(GP_FBM gp_fbm/main.cpp gp_fbm/header.h)
target_link_libraries(GP_FBM PUBLIC Methods)
target_compile_definitions(GP_FBM PRIVATE OPATH="${PROJECT_SOURCE_DIR}/gp_fbm")

#########################################

add_executable(GP_FBM_SUB gp_fbm_substrate/main.cpp gp_fbm_substrate/header.h)
target_link_libraries(GP_FBM_SUB PUBLIC Methods)
target_compile_definitions(GP_FBM_SUB PRIVATE OPATH="${PROJECT_SOURCE_DIR}/gp_fbm_substrate")