project(srslog)

cmake_minimum_required(VERSION 2.8)
include_directories(st)
link_directories(st)
aux_source_directory(. SRC_LIST)

set(OBJ_EXE_NAME screenshot)
add_executable(${OBJ_EXE_NAME} ${SRC_LIST})
target_link_libraries(${OBJ_EXE_NAME} libst.a)
