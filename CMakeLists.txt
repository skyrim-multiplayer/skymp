if(WIN32)
  set(VCPKG_TARGET_TRIPLET "x64-windows-sp")
else()
  set(VCPKG_TARGET_TRIPLET "x64-linux")
endif()

set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/overlay_triplets")
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/overlay_ports")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

cmake_minimum_required(VERSION 3.19.1)
project(skymp)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

add_subdirectory(chakra-wrapper)
add_subdirectory(skyrim-platform)
add_subdirectory(skymp5-client)
add_subdirectory(skymp5-front)