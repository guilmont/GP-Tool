cmake_minimum_required(VERSION 3.10)
project(Engine3D VERSION 1.2.0 DESCRIPTION "Quick renderer to facilitated visualization")


# Includes all the OpenGL dependencies
add_subdirectory(glad)

# Includes Dear Imgui library
add_subdirectory(imgui)

# We need glfw and glm for the renderer
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Configuration file (mostly for shaders location)
add_library(Engine3D SHARED "src/object.cpp"
							"src/shader.cpp"
							"src/camera.cpp" 
							"src/renderer.cpp"
							"src/texture.cpp"
							"src/framebuffer.cpp"
							"src/gevent.cpp")

target_include_directories(Engine3D 
	PUBLIC 
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(Engine3D PUBLIC glad glfw imgui objLoader)


# Configuring some files based on build
configure_file(Engine3D.pc.in Engine3D.pc @ONLY)

