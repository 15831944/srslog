project(srslog)

cmake_minimum_required(VERSION 2.8)

include_directories(${CMAKE_SOURCE_DIR}/sdks/st)
include_directories(${CMAKE_SOURCE_DIR}/sdks/jvtoken)
link_directories(${CMAKE_SOURCE_DIR}/sdks/st)
link_directories(${CMAKE_SOURCE_DIR}/sdks/jvtoken)

aux_source_directory(. SRC_LIST)

set(OBJ_EXE_NAME charge)
add_executable(${OBJ_EXE_NAME} ${SRC_LIST})

target_link_libraries(${OBJ_EXE_NAME} libst.a)
target_link_libraries(${OBJ_EXE_NAME} libhiredis.a)
target_link_libraries(${OBJ_EXE_NAME} libjvtoken.a)
target_link_libraries(${OBJ_EXE_NAME} pthread)
