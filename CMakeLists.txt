cmake_minimum_required(VERSION 3.26)

project(polybox C CXX)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/build;${CMAKE_CURRENT_SOURCE_DIR}")
set(PROJ_ROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

include(common)

add_subdirectory(source)
add_subdirectory(source/common_lib)
add_subdirectory(source/third_party/FreeType)
add_subdirectory(source/third_party/luau)
add_subdirectory(source/third_party/lz4)
