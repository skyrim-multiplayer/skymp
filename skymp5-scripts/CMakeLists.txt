cmake_minimum_required(VERSION 3.19.1)
project(skymp5-scripts)

include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library.cmake)

add_papyrus_library(
    NAME skymp5-scripts
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/Source/Scripts
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/dist/server/data/scripts
)
