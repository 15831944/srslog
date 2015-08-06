project(jvmbs)
cmake_minimum_required(VERSION 2.8)

include_directories(src/base)

include_directories(sdk/st)
link_directories(sdk/st)

aux_source_directory(src SRC_LIST)
aux_source_directory(src/base SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME} libst.a)
