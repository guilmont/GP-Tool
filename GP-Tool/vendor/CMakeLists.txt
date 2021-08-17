cmake_minimum_required(VERSION 3.10.0)

###########################################################
## GRENDER ################################################

add_subdirectory(GRender/GRender)

###########################################################
## JSONCPP ################################################

add_library(jsoncpp
    "jsoncpp/src/lib_json/json_tool.h"
    "jsoncpp/src/lib_json/json_reader.cpp"
    "jsoncpp/src/lib_json/json_valueiterator.inl"
    "jsoncpp/src/lib_json/json_value.cpp"
    "jsoncpp/src/lib_json/json_writer.cpp"
)

target_include_directories(jsoncpp PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/jsoncpp/include")
