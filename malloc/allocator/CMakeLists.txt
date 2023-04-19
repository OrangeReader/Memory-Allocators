message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# 通用的allocator静态库, 底层实现由其他静态库实现
add_definitions(-DDEBUG_MALLOC)

#add_definitions(-DIMPLICIT_FREE_LIST)   # 隐式空闲链表 + 8-Byte free block
add_definitions(-DEXPLICIT_FREE_LIST)   # 显式空闲链表 + 8-Byte free block

add_library(allocator STATIC allocator.cpp block.cpp)