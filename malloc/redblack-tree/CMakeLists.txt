message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# allocator的底层实现: 隐式空闲链表
add_library(redblack-tree STATIC redblack-tree.cpp)