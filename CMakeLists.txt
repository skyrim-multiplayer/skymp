if(WIN32)
  set(VCPKG_TARGET_TRIPLET "x64-windows-sp")
else()
  set(VCPKG_TARGET_TRIPLET "x64-linux")
endif()

set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/overlay_triplets")
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/overlay_ports")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

# File from vcpkg submodule. This indicates inability to find this file or checkout submodules.
if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  set(msg "${CMAKE_TOOLCHAIN_FILE} doesn't exist. It seems that vcpkg submodule is not initialized.")
  set(msg "${msg}\nUse commands below to initialize:")
  set(msg "${msg}\n  git submodule init")
  set(msg "${msg}\n  git submodule update")
  message(FATAL_ERROR "${msg}")
endif()

cmake_minimum_required(VERSION 3.18.4)
project(skymp)
enable_testing()

option(UNIT_DATA_DIR "Path to directory with Skyrim.esm and other data files required for testing" OFF)
option(SKYRIM_DIR "Path to Skyrim, would be used for tests if UNIT_DATA_DIR is not specified" OFF)

if(NOT SKYRIM_DIR)
  message(STATUS "SKYRIM_DIR is not specified. Using prebuilt Papyrus scripts instead of building from sources")
endif()
string(REPLACE "\\" "/" SKYRIM_DIR "${SKYRIM_DIR}")

if(SKYRIM_DIR)
  if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (UNIX)
  # TODO: Fix all errors instead of passing -fpermissive
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

add_subdirectory(chakra-wrapper)
add_subdirectory(skyrim-platform)
add_subdirectory(skymp5-client)
add_subdirectory(skymp5-front)
add_subdirectory(skymp5-functions-lib)
add_subdirectory(skymp5-scripts)
add_subdirectory(client-deps)
add_subdirectory(skymp5-server)
