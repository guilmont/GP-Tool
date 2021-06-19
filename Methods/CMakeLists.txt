cmake_minimum_required(VERSION 3.10.0)


find_package(Threads REQUIRED)


# Including third party software
add_subdirectory(vendor)

# Creating shared project with out methods
project(Methods)

add_library(Methods SHARED
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


target_precompile_headers(Methods PUBLIC "include/header.h")

target_include_directories(Methods PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(Methods PUBLIC glm_ eigen pugixml Threads::Threads)
target_compile_definitions(Methods PRIVATE GBUILD_DLL)
