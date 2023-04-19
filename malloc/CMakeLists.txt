message(STATUS "Current source dir: ${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(small-list)
add_subdirectory(implicit-list)
add_subdirectory(explicit-list)
add_subdirectory(redblack-tree)

add_subdirectory(allocator)