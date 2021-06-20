cmake_minimum_required(VERSION 3.10.0)
project(Tests)


add_executable(Traj ${PROJECT_SOURCE_DIR}/trajectories/main.cpp)
target_link_libraries(Traj PUBLIC Methods)

target_compile_definitions(Traj PRIVATE OPATH="${PROJECT_SOURCE_DIR}/trajectories")

if (WIN32)
	set_property(TARGET Traj PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()