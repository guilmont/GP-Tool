cmake_minimum_required(VERSION 3.0.0)

project(batch)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin")

set(GPMethods_DIR "C:/Users/GM Oliveira/Desktop/TestGP/GPLibs/lib/cmake")  # Insert you GP-Tool cmake path in here

find_package(Threads REQUIRED)    # Multi-threaded component
find_package(GPMethods REQUIRED)  # GP-FBM methods
find_package(GDManager REQUIRED)  # My solution to store results, you can choose whatever you like

add_executable(batch batch.cpp)
target_link_libraries(batch PUBLIC GPMethods GDManager Threads::Threads)

