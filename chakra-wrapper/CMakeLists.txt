cmake_minimum_required(VERSION 3.18.4)
project(chakra-wrapper)

add_library(chakra-wrapper INTERFACE)
target_include_directories(chakra-wrapper INTERFACE ${CMAKE_CURRENT_LIST_DIR})
