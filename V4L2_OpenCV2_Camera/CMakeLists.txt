set(CMAKE_BUILD_TYPE "Debug")
#required cmake minimum version
cmake_minimum_required(VERSION 2.8)

STRING(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR})
set(SRC_DIR_NAME ${CURRENT_FOLDER})

project(${SRC_DIR_NAME})

SET(LDFLAGS "-pie -ldl -nostdlib")
SET(CFLAGS "-w -Wno-unknown-pragmas")
SET(CFLAGS "-std=c++11")
SET(CFLAGS "-O3 -fno-tree-pre -fno-strict-aliasing")
SET(CFLAGS "-ftree-vectorize -ffast-math -funsafe-math-optimizations -fsingle-precision-constant")
SET(CMAKE_C_FLAGS "${CFLAGS} ${LDFLAGS}")

#添加配置
include (CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_PROCESSOR arm)
SET(ARCH arm)

SET(CMAKE_EXE_LINKER_FLAGS "-lpthread -lrt -ldl") # note dlopen pthread_create
SET(CMAKE_CROSSCOMPILING 1)

SET(TOOLCHAIN_DIR "/home/taikoto/workspace/9tripod/buildroot-2017.11.2/output/host/opt/ext-toolchain")
SET(LINKER_SCRIPT ${TOOLCHAIN_DIR}/bin/arm-cortex_a9-linux-gnueabi-ld)
SET(OpenCV_DIR /home/taikoto/opencv-2.4.13/opencv-build-dir)

CMAKE_FORCE_C_COMPILER(arm-cortex_a9-linux-gnueabi-gcc NONE)
CMAKE_FORCE_CXX_COMPILER(arm-cortex_a9-linux-gnueabi-g++ NONE)

SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-cortex_a9-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-cortex_a9-linux-gnueabi-g++)

# find_library(WORK_LIB
#    NAMES "libc.so"
#    PATHS "${LIB_DIR}"
#)

#link_directories(
#    /lib
#)

find_package(OpenCV REQUIRED)
message(${OpenCV_INCLUDE_DIRS})

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR} # build dir
    ${CMAKE_CURRENT_SOURCE_DIR} # c_target dir
    ${OpenCV_INCLUDE_DIRS}
)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SRC_LIST)
add_executable(${SRC_DIR_NAME} ${SRC_LIST})

execute_process(COMMAND echo ${CMAKE_CURRENT_SOURCE_DIR})
execute_process(COMMAND echo ${CURRENT_FOLDER})
execute_process(COMMAND echo ${CMAKE_CURRENT_BINARY_DIR})
execute_process(COMMAND echo ${SRC_DIR_NAME})

TARGET_LINK_LIBRARIES(${SRC_DIR_NAME}
#    libopencv_highgui.so
#    libopencv_core.so
#    libopencv_imgproc.so
#    libopencv_video.so
#    libopencv_objdetect.so
    ${OpenCV_LIBS}
)
