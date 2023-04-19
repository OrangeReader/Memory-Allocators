message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# allocator的底层实现: 显式空闲链表
add_definitions(-DDEBUG_MALLOC -DEXPLICIT_FREE_LIST)
add_library(explicit-list STATIC explicit-list.cpp)