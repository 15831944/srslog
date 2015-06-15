project(srslog)

cmake_minimum_required(VERSION 2.8)
include_directories(st)
link_directories(st)
link_directories(/usr/lib64/mysql/)
aux_source_directory(. SRC_LIST)

set(OBJ_EXE_NAME charge)
add_executable(${OBJ_EXE_NAME} ${SRC_LIST})
target_link_libraries(${OBJ_EXE_NAME} libst.a)
target_link_libraries(${OBJ_EXE_NAME} libhiredis.a)
target_link_libraries(${OBJ_EXE_NAME} libanti_streal_link_token.a)
target_link_libraries(${OBJ_EXE_NAME} mysqlclient)
