cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

project(GPTests)

## Tests the methods written for aligning images
add_executable(testAlign testAlign.cpp testHeader.h)
target_link_libraries(testAlign PUBLIC gtest_main GPMethods jsoncpp)
target_compile_definitions(testAlign PRIVATE GTEST_PATH="${PROJECT_SOURCE_DIR}/output")

## Tests the algorithms written using Gaussian Processes and Fractional Brownian Motion
add_executable(testGP testGP.cpp testHeader.h)
target_link_libraries(testGP PUBLIC gtest_main GPMethods jsoncpp)
target_compile_definitions(testGP PRIVATE GTEST_PATH="${PROJECT_SOURCE_DIR}/output")

## Generates some data to test the batching function of GP-Tool (graphical interface)
add_executable(testBatch testBatch.cpp testHeader.h)
target_link_libraries(testBatch PUBLIC gtest_main GPMethods jsoncpp)
target_compile_definitions(testBatch PRIVATE 
	GTEST_SINGLE="${PROJECT_SOURCE_DIR}/batchData_Single"
	GTEST_COUPLED="${PROJECT_SOURCE_DIR}/batchData_Coupled"
)

## Tests for all the denoising filters
add_executable(testFilters testFilters.cpp)
target_link_libraries(testFilters PUBLIC gtest_main GPMethods)


include(GoogleTest)
gtest_discover_tests(testAlign testGP testBatch)