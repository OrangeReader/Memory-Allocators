cmake_minimum_required(VERSION 3.24)
project(MyMalloc)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(algorithm)
add_subdirectory(malloc)

add_definitions(-DDEBUG_MALLOC)

add_executable(MyMalloc test-malloc.cpp)

# 采用implicit-list 实现的 allocator
#add_definitions(-DIMPLICIT_FREE_LIST)
#target_link_libraries(MyMalloc PRIVATE allocator implicit-list linked-list)

# 采用explicit-list 实现的 allocator
add_definitions(-DEXPLICIT_FREE_LIST)
target_link_libraries(MyMalloc PRIVATE allocator explicit-list linked-list)