cmake_minimum_required(VERSION 3.24)
project(malloc)

set(CMAKE_CXX_STANDARD 11)

include_directories("./headers")

add_executable(linkedlist algorithm/linked-list.cpp mains/implicit-list.cpp)
