message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

add_library(linked-list STATIC linked-list.cpp)

# 链接 linked-list 这个库的 target可以 #include 该头文件
#target_include_directories(linked-list PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})