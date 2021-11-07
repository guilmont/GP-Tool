cmake_minimum_required(VERSION 3.10.0)
project(GPMethods)

set(GP_FBM_INC "include/GP_FBM")
set(GP_EXPORT "${PROJECT_NAME}Config")


# Including third party software
find_package(Threads REQUIRED)
add_subdirectory(vendor)

# Creating shared project with out methods
add_library(${PROJECT_NAME} SHARED
	"include/header.h"
	"include/gtiffer.h"     "src/gtiffer.cpp"
	"include/goptimize.h"   "src/goptimize.cpp"
	"include/movie.h"       "src/movie.cpp"
	"include/metadata.h"    "src/metadata.cpp"
	"include/align.h"       "src/align.cpp"
	"include/spot.h"        "src/spot.cpp"
	"include/trajectory.h"  "src/trajectory.cpp"
	"include/gp_fbm.h"      "src/gp_fbm.cpp"
)

if (GP_PRECOMPILED_HEADERS)
	target_precompile_headers(${PROJECT_NAME} PUBLIC "include/header.h")
endif()


target_compile_definitions(${PROJECT_NAME} PRIVATE GBUILD_DLL)
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> 
    $<INSTALL_INTERFACE:${GP_FBM_INC}>
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    OUTPUT_NAME_RELEASE ${PROJECT_NAME}
    OUTPUT_NAME_RELWITHDEBINFO ${PROJECT_NAME}_RelWithDebInfo
    OUTPUT_NAME_DEBUG ${PROJECT_NAME}_Debug
)

target_link_libraries(${PROJECT_NAME} PUBLIC eigen pugixml Threads::Threads)

file(GLOB INC_FILES "include/*.h")
install(FILES ${INC_FILES} DESTINATION ${GP_FBM_INC})

install(TARGETS ${PROJECT_NAME}
    EXPORT ${GP_EXPORT}
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)

install(EXPORT ${GP_EXPORT} FILE ${GP_EXPORT}.cmake DESTINATION lib/cmake)
