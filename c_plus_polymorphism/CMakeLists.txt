set(CMAKE_BUILD_TYPE "Debug")
#required cmake minimum version
cmake_minimum_required(VERSION 2.8)


STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR_NAME ${CURRENT_FOLDER})
# SET(WORKSPACE_DIR "/workspace/test")

project(${SRC_DIR_NAME})

# find_library(WORK_LIB
#    NAMES "libc.so"
#    PATHS "${LIB_DIR}"
#)

#link_directories(
#    ${WORKSPACE_DIR}/c_target/system/lib64
#)

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # build dir
    ${CMAKE_CURRENT_SOURCE_DIR} # c_target dir
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
add_executable(${SRC_DIR_NAME} ${SRC_LIST})

#TARGET_LINK_LIBRARIES(led_demo 
#    ${WORKSPACE_DIR}/c_target/system/lib64
#)


execute_process(COMMAND echo ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND echo ${CURRENT_FOLDER})
execute_process(COMMAND echo ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND echo ${SRC_DIR_NAME})
