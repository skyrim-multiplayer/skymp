cmake_minimum_required(VERSION 3.17)
project(skyrim_multiplayer)

option(SKYRIM_DIR "Path to Skyrim" OFF)
if (NOT SKYRIM_DIR)
  message(FATAL_ERROR "Please specify SKYRIM_DIR")
endif()
if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
  message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
endif()

include(src/cmake/cmake_generate.cmake)

# Clear CMake cache after changing this
# (also for subprojects like platform_se)
set(VCPKG_REVISION f29a191d0afccc3ed6f481283d6d15e0186096ae)

# Download vcpkg dependencies
include(pmm.cmake)
set(ports
  chakracore:x86-windows chakracore:x64-windows
  nlohmann-json:x86-windows nlohmann-json:x64-windows
  cpp-httplib:x86-windows cpp-httplib:x64-windows
  zlib:x86-windows zlib:x64-windows 
)
foreach(port ${ports})
  message(STATUS "Installing ${port}")
  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES ${port})
endforeach()
if ("${_VCPKG_ROOT_DIR}" STREQUAL "")
  message(FATAL_ERROR "Expected _VCPKG_ROOT_DIR variable to be set by pmm")
endif()

cmake_generate(
  NAME platform_se
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/platform_se"
  BINARY_DIR "${CMAKE_BINARY_DIR}/_platform_se"
  ARCHITECTURE "x64"
  GENERATOR "${CMAKE_GENERATOR}"
  VARIABLES
    "SKYRIM_MP_ROOT=${CMAKE_SOURCE_DIR}"
    "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "VCPKG_ROOT=${_VCPKG_ROOT_DIR}"
    "CMAKE_TOOLCHAIN_FILE=${_VCPKG_ROOT_DIR}\\scripts\\buildsystems\\vcpkg.cmake"
    "SKYRIM_DIR=${SKYRIM_DIR}"
)
