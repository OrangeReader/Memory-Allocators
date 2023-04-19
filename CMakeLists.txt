cmake_minimum_required(VERSION 3.24)
project(MyMalloc)

set(CMAKE_CXX_STANDARD 11)

include_directories(${CMAKE_SOURCE_DIR}/include)

add_subdirectory(algorithm)
add_subdirectory(malloc)

# ==================================== #
#           for test malloc            #
# ==================================== #

add_executable(test-malloc test-malloc.cpp)

# 链接时，让有依赖的处于前面，被依赖的处于后面

# 采用implicit-list 实现的 allocator
#add_definitions(-DIMPLICIT_FREE_LIST)
#target_link_libraries(test-malloc PRIVATE allocator implicit-list small-list linked-list)

# 采用explicit-list 实现的 allocator
add_definitions(-DEXPLICIT_FREE_LIST)
target_link_libraries(test-malloc PRIVATE allocator explicit-list small-list linked-list)

# 采用red black tree 实现的 allocator
add_definitions(-DREDBLACK_TREE)
target_link_libraries(test-malloc PRIVATE allocator redblack-tree rbt explicit-list small-list linked-list utils)

# ==================================== #
#           for test rbt               #
# ==================================== #
#add_executable(test-rbt test-rbt.cpp)
#target_link_libraries(test-rbt PRIVATE rbt utils)