cmake_minimum_required(VERSION 3.24)
project(main)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(algorithm)
add_subdirectory(malloc)

add_definitions(-DDEBUG_MALLOC)

add_executable(main test-malloc.cpp)

# 采用implicit-list 实现的 allocator
target_link_libraries(main PRIVATE allocator implicit-list linked-list)
