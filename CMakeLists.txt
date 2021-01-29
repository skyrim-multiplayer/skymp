cmake_minimum_required(VERSION 3.17)
project(skyrim_multiplayer)

option(SKYRIM_DIR "Path to Skyrim" OFF)
if (NOT SKYRIM_DIR)
  message(WARNING "SKYRIM_DIR is not specified. You will not be able to compile Papyrus scripts")
  message(WARNING "It's ok for CI/CD, but not ok for the tool development")
endif()

if (SKYRIM_DIR)
  if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()
endif()

option(CEF_DIR "Path to CEF dist" OFF)
if (NOT CEF_DIR)
  set(CEF_DIR_DEFAULT ${CMAKE_SOURCE_DIR}/cef_download/cef_binary_84.3.7+g97011bc+chromium-84.0.4147.89_windows64)
  message(WARNING "CEF_DIR is not specified, defaulting to '${CEF_DIR_DEFAULT}'")
  set(CEF_DIR ${CEF_DIR_DEFAULT})
endif()

set(expected_readme_hash dbaa71607debbba411f9ce4c8e28ca52ba98676ebf11db25d2ba0e47b686c7ec9b30bfc34bdd7ff665e3b79cb863c85639585bd6d945c86f1b28fee548452c47)
file(SHA512 ${CEF_DIR}/README.txt readme_hash)
if (NOT "${readme_hash}" STREQUAL expected_readme_hash)
  message(FATAL_ERROR "Unexpected CEF README hash\n\tExpected: ${expected_readme_hash}\n\tActual: ${readme_hash}")
endif()
list(APPEND CMAKE_MODULE_PATH "${CEF_DIR}/cmake")

include(src/cmake/cmake_generate.cmake)

cmake_generate(
  NAME platform_se
  SOURCE_DIR "${CMAKE_SOURCE_DIR}/src/platform_se"
  BINARY_DIR "${CMAKE_BINARY_DIR}/_platform_se"
  ARCHITECTURE "x64"
  GENERATOR "${CMAKE_GENERATOR}"
  VARIABLES
    "SKYRIM_MP_ROOT=${CMAKE_SOURCE_DIR}"
    "CMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}"
    "SKYRIM_DIR=${SKYRIM_DIR}"
    "CEF_DIR=${CEF_DIR}"
)
