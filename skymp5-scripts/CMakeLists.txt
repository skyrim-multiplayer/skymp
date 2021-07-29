cmake_minimum_required(VERSION 3.19.1)
project(skymp5-scripts)

include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library_ck.cmake)

add_papyrus_library_ck(
    NAME skymp5-scripts
    DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/psc
    OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/pex
    COMPILER_EXECUTABLE_PATH "${SKYRIM_DIR}/Papyrus compiler/PapyrusCompiler.exe"
)

add_custom_command(
    TARGET skymp5-scripts POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/pex ${CMAKE_BINARY_DIR}/dist/server/data/scripts
)
