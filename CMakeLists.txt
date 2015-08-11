#head
project(jvmbs)
cmake_minimum_required(VERSION 2.8)

#include
include_directories(src/base)
include_directories(src/json)
include_directories(src/conf)

#sdks
include_directories(sdk/st)
link_directories(sdk/st)
include_directories(sdk/json)
link_directories(sdk/redis)

#src list
aux_source_directory(src SRC_LIST)
aux_source_directory(src/base SRC_LIST)
aux_source_directory(src/conns/record SRC_LIST)
aux_source_directory(src/conns/redis_process SRC_LIST)
aux_source_directory(src/json SRC_LIST)
aux_source_directory(src/conf SRC_LIST)

#exe
add_executable(${PROJECT_NAME} ${SRC_LIST})

#link libraries
target_link_libraries(${PROJECT_NAME} libst.a)
target_link_libraries(${PROJECT_NAME} libhiredis.a)
target_link_libraries(${PROJECT_NAME} libjv_parse_json.a)
target_link_libraries(${PROJECT_NAME} pthread)
