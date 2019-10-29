#required cmake minimum version
cmake_minimum_required(VERSION 2.8)

#required cmake project
project(libHelloSLAM)

#add executable
#add_executable(helloSLAM helloSLAM.cpp)

#adb share lib
add_library(hello_shared SHARED libHelloSLAM.cpp)

#add executable
add_executable(useHello useHello.cpp)
target_link_libraries(useHello hello_shared)

