cmake_minimum_required(VERSION 3.16)
project(monitor_controller)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(NOMINMAX)
SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread")

include_directories("libs")
add_executable(monitor_controller main.cpp)
