cmake_minimum_required(VERSION 3.10)
project(bmptool C)

set(CMAKE_C_STANDARD 11)

add_executable(bmptool main.c bmp.c bmp.h)
target_link_libraries(bmptool m)