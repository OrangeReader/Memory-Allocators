message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

# 通用的allocator静态库, 底层实现由其他静态库实现
add_definitions(-DDEBUG_MALLOC)
add_library(allocator STATIC allocator.cpp)

#target_include_directories(allocator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})