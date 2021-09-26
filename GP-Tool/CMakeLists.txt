cmake_minimum_required(VERSION 3.10.0)

# Including vendors
set(ASSETS_DIR "assets/GP-Tool")
add_subdirectory(vendor)

project(GP-Tool)
add_executable(${PROJECT_NAME}
	"include/gpch.h"
	"include/gptool.h"      "src/gptool.cpp"
	"include/plugin.h" 
	"include/moviePlugin.h"  "src/moviePlugin.cpp" 
	"include/alignPlugin.h"  "src/alignPlugin.cpp" 
	"include/trajPlugin.h"   "src/trajPlugin.cpp" 
	"include/circle.h"       "src/circle.cpp"
	"include/roi.h"          "src/roi.cpp"
	"include/gpPlugin.h"     "src/gpPlugin.cpp"
	"src/app.cpp"
)


target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PUBLIC GRender jsoncpp GPMethods)
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

if (GP_PRECOMPILED_HEADERS)
	target_precompile_headers(${PROJECT_NAME} PRIVATE "include/gpch.h")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	_CRT_SECURE_NO_WARNINGS
	INSTALL_PATH="${CMAKE_INSTALL_PREFIX}"
	ASSETS="${ASSETS_DIR}"
)

file(COPY ${PROJECT_SOURCE_DIR}/assets/ DESTINATION ${CMAKE_SOURCE_DIR}/${ASSETS_DIR})

install(TARGETS ${PROJECT_NAME} 
	CONFIGURATIONS Release
		DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets 
	CONFIGURATIONS Release 
		DESTINATION ${CMAKE_INSTALL_PREFIX}
)