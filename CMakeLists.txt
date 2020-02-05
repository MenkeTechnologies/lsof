cmake_minimum_required(VERSION 3.15)
project(3530bst)

set(CMAKE_CXX_STANDARD 14)

include_directories(${PROJECT_SOURCE_DIR})
file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.c)

add_executable(lsof ${SRC_FILES})