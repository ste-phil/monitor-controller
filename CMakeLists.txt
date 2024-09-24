cmake_minimum_required(VERSION 3.24)
project(monitor_controller)

set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(NOMINMAX)

include_directories("libs")
add_executable(monitor_controller main.cpp)
