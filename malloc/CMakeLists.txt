add_subdirectory(implicit-list)
add_subdirectory(allocator)

## 设置静态库a的路径和名称
#set(LIB_A_PATH ${CMAKE_CURRENT_SOURCE_DIR}/allocator)
#set(LIB_A_NAME allocator)
#
## 设置静态库b的路径和名称
#set(LIB_B_PATH ${CMAKE_CURRENT_SOURCE_DIR}/implicit-list)
#set(LIB_B_NAME implicit-list)
#
## 设置合并后静态库c的名称
#set(LIB_MERGE_NAME malloc)
#
## 合并静态库a和b为静态库c
#add_library(${LIB_MERGE_NAME} STATIC IMPORTED)
#set_target_properties(${LIB_MERGE_NAME} PROPERTIES IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/lib${LIB_C_NAME}.a)
#target_link_libraries(${LIB_MERGE_NAME} INTERFACE ${LIB_A_NAME} ${LIB_B_NAME})
#
#message("get merge lib")