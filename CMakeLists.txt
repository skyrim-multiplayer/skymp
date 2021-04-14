set(VCPKG_TARGET_TRIPLET "x64-windows-sp")
set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/overlay_triplets")
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/overlay_ports")
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

cmake_minimum_required(VERSION 3.19.1)
project(platform_se)

add_executable(vs_startup_project src/platform_se/vs_startup_project/main.cpp)
set_target_properties(vs_startup_project PROPERTIES WIN32_EXECUTABLE true CXX_STANDARD 17)
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT vs_startup_project) 

option(SKYRIM_DIR "Path to Skyrim" OFF)
if (NOT SKYRIM_DIR)
  message(STATUS "SKYRIM_DIR is not specified. Using prebuilt Papyrus scripts instead of building from sources")
endif()

if (SKYRIM_DIR)
  if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()
endif()

set(DEV_SERVICE_CFG_PATH "tools/dev_service/config.js")
file(WRITE ${DEV_SERVICE_CFG_PATH} "")
file(APPEND ${DEV_SERVICE_CFG_PATH} "module.exports = {\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "  SkyrimSEFolder:\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "    '${SKYRIM_DIR}',\n")
file(APPEND ${DEV_SERVICE_CFG_PATH} "};\n")

set(README_HASH_EXPECTED 
  dbaa71607debbba411f9ce4c8e28ca52ba98676ebf11db25d2ba0e47b686c7ec9b30bfc34bdd7ff665e3b79cb863c85639585bd6d945c86f1b28fee548452c47
)
set(CEF_URL "https://cef-builds.spotifycdn.com/cef_binary_84.3.7%2Bg97011bc%2Bchromium-84.0.4147.89_windows64.tar.bz2")

file(GLOB CEF_ROOT ${CMAKE_BINARY_DIR}/cef/*)
if (EXISTS ${CEF_ROOT}/README.txt)
  file(SHA512 ${CEF_ROOT}/README.txt README_HASH)
endif()

if (NOT "${README_HASH}" STREQUAL "${README_HASH_EXPECTED}")
  message(STATUS "Unexpected CEF README hash\n\tExpected: ${README_HASH_EXPECTED}\n\tActual: ${README_HASH}")
  message(STATUS "Installing CEF")
  file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/cef)
  message(STATUS "Downloading...")
  file(DOWNLOAD ${CEF_URL} ${CMAKE_BINARY_DIR}/cef.tar.bz2)
  message(STATUS "Extracting archive...")
  file(ARCHIVE_EXTRACT INPUT ${CMAKE_BINARY_DIR}/cef.tar.bz2 DESTINATION ${CMAKE_BINARY_DIR}/cef)
  file(GLOB CEF_ROOT ${CMAKE_BINARY_DIR}/cef/*)
  file(REMOVE ${CMAKE_BINARY_DIR}/cef.tar.bz2)
endif()
message(STATUS "Found CEF: ${CEF_ROOT}")
list(APPEND CMAKE_MODULE_PATH "${CEF_ROOT}/cmake")

set(SKYRIM_MP_ROOT ${CMAKE_SOURCE_DIR})
add_subdirectory(src/platform_se _platform_se)