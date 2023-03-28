message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# allocator的底层实现: 隐式空闲链表
add_definitions(-DDEBUG_MALLOC -DIMPLICIT_FREE_LIST)
add_library(implicit-list STATIC implicit-list.cpp)