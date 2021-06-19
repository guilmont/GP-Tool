cmake_minimum_required(VERSION 3.10.0)

###########################################################
## GRENDER ################################################

add_subdirectory(GRender)

###########################################################
## JSONCPP ################################################

add_library(jsoncpp STATIC
    "jsoncpp/src/lib_json/json_reader.cpp"
    "jsoncpp/src/lib_json/json_writer.cpp"
    "jsoncpp/src/lib_json/json_value.cpp"
)

target_include_directories(jsoncpp PUBLIC "jsoncpp/include")
