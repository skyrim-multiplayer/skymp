cmake_minimum_required(VERSION 3.17)
project(skyrim_multiplayer)

include(src/cmake/cmake_generate.cmake)

if ("${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
  message(FATAL_ERROR "Missing toolchain file")
endif()

cmake_generate(
  NAME platform_se
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/platform_se"
  BINARY_DIR "${CMAKE_BINARY_DIR}/_platform_se"
  ARCHITECTURE "x64"
  GENERATOR "${CMAKE_GENERATOR}"
  VARIABLES "SKYRIM_MP_ROOT=${CMAKE_SOURCE_DIR}" "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
)
