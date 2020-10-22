set(CMAKE_BUILD_TYPE "Debug")
#required cmake minimum version
cmake_minimum_required(VERSION 2.8)

#required cmake project
project(led_demo)

#add executable
add_executable(led_demo board.c led_demo.c)
