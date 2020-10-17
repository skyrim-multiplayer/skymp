include(${CMAKE_SOURCE_DIR}/cmake/third_party/pmm.cmake)
# Clear CMake cache after changing this
# (also for subprojects like platform_se)
set(VCPKG_REVISION 790910f79f653978f90aadd958abf3c407215552)
set(triplet x64-windows-static)

if (UNIX)
  set(VCPKG_PATH "/vcpkg")
  if (NOT EXISTS "${VCPKG_PATH}/bootstrap-vcpkg.sh")
    message(FATAL_ERROR "VCPKG_PATH ('${VCPKG_PATH}') is not a valid vcpkg path")
  endif()
else()
  # Download vcpkg dependencies
  set(ports
    catch2:${triplet}
    sparsepp:${triplet}
    nlohmann-json:${triplet}
    spdlog:${triplet}
    slikenet:${triplet}
    boost-bimap:${triplet}
    zlib:${triplet}
    sqlite-orm:${triplet}
    sqlpp11:${triplet}
    sqlpp11-connector-sqlite3:${triplet}
  )
  set(VCPKG_PATH ${_VCPKG_ROOT_DIR})
endif()

if (WIN32)
  list(APPEND ports simdjson:${triplet})
endif()
foreach(port ${ports})
  message(STATUS "Installing ${port}")
  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES ${port})
endforeach()
if ("${VCPKG_PATH}" STREQUAL "")
  message(FATAL_ERROR "Expected VCPKG_PATH variable to be set")
endif()

# TODO: Check SHA512
include(${CMAKE_SOURCE_DIR}/cmake/third_party/cmake_scripts_collection.cmake)

if (NOT CPPCOV)
  message(WARNING "CPPCOV is not set, coverage for C++ code is disabled")
endif()

if(WIN32)
  option(SKYRIM_DIR "Path to Skyrim" OFF)
  if (NOT SKYRIM_DIR)
    message(WARNING "SKYRIM_DIR is not specified, MpClientPlugin will not be installed")
  endif()
  if (SKYRIM_DIR AND NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()

  file(GLOB_RECURSE src "client/*" "${CMAKE_SOURCE_DIR}/.clang-format")
  add_library(MpClientPlugin SHARED ${src})
  target_link_libraries(MpClientPlugin PUBLIC mp_common)
  set_target_properties(MpClientPlugin PROPERTIES OUTPUT_NAME "MpClientPlugin")
  apply_default_settings(TARGETS MpClientPlugin)
  list(APPEND VCPKG_DEPENDENT MpClientPlugin)

  if(SKYRIM_DIR)
    add_custom_command(
      TARGET MpClientPlugin POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MpClientPlugin> ${SKYRIM_DIR}/Data/SKSE/plugins
    )
  endif()
endif()

set(OPENCPPCOV_DIR "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib_espm)
list(APPEND VCPKG_DEPENDENT espm)
apply_default_settings(TARGETS espm)

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/unit/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_executable(unit ${src})
target_link_libraries(unit PRIVATE server_guest_lib)
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mp_common")
apply_default_settings(TARGETS unit)
list(APPEND VCPKG_DEPENDENT unit)
target_compile_definitions(unit PRIVATE TEST_PEX_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files/standard_scripts\")

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/mp_common/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(mp_common STATIC ${src})
target_compile_definitions(mp_common PUBLIC MAX_PLAYERS=100)
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/mp_common")
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/third_party")
apply_default_settings(TARGETS mp_common)
list(APPEND VCPKG_DEPENDENT mp_common)
if (WIN32)
  target_link_libraries(mp_common PUBLIC ws2_32.lib)
endif()

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/server_guest_lib/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(server_guest_lib STATIC ${src})
target_include_directories(server_guest_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/server_guest_lib")
apply_default_settings(TARGETS server_guest_lib)
list(APPEND VCPKG_DEPENDENT server_guest_lib)
target_link_libraries(server_guest_lib PUBLIC mp_common espm papyrus-vm-lib)
if (WIN32)
  file(COPY
    ${CMAKE_SOURCE_DIR}/cmake/ctest.bat
    ${CMAKE_SOURCE_DIR}/cmake/before_ctest.cmake
    DESTINATION ${CMAKE_BINARY_DIR}
  )
  file(APPEND ${CMAKE_BINARY_DIR}/before_ctest.cmake
    "\n\n# This file was copied from cmake/before_ctest.cmake, do not modify"
  )
endif()

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/papyrus_vm_lib/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(papyrus-vm-lib STATIC ${src})
apply_default_settings(TARGETS papyrus-vm-lib)
target_include_directories(papyrus-vm-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/papyrus_vm_lib")
if (UNIX)
  target_link_libraries(papyrus-vm-lib PUBLIC stdc++fs)
endif()

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/papyrus_vm_main/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_executable(papyrus-vm-main ${src})
target_link_libraries(papyrus-vm-main PUBLIC papyrus-vm-lib)
list(APPEND VCPKG_DEPENDENT papyrus-vm-main)
set(VM_MAIN_PATH ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/papyrus-vm-main.exe)
apply_default_settings(TARGETS papyrus-vm-main)

if (NOT SKYRIM_DIR)
  message(WARNING "SKYRIM_DIR is not specified, Papyrus sources will not be built")
else()
  include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library.cmake)
  add_papyrus_library(NAME papyrus_test_files DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/tests_unit.cmake)

integrate_vcpkg(
  TARGETS ${VCPKG_DEPENDENT}
  VCPKG_PATH "${VCPKG_PATH}"
)

if(UNIX)
  foreach(target ${VCPKG_DEPENDENT})
    target_link_libraries(${target} PUBLIC pthread)
  endforeach()
endif()

# For now we are unable to install simdjson from vcpkg on linux
if(UNIX)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/simdjson)
  file(DOWNLOAD
    https://raw.githubusercontent.com/simdjson/simdjson/d0ce2f0b5a6b93707bf6872cc591f32cf5d9eef2/singleheader/simdjson.h
    ${CMAKE_CURRENT_BINARY_DIR}/simdjson/simdjson.h
  )
  file(DOWNLOAD
    https://raw.githubusercontent.com/simdjson/simdjson/d0ce2f0b5a6b93707bf6872cc591f32cf5d9eef2/singleheader/simdjson.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/simdjson/simdjson.cpp
  )
  foreach(target ${VCPKG_DEPENDENT})
    target_sources(${target} PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/simdjson/simdjson.h
      ${CMAKE_CURRENT_BINARY_DIR}/simdjson/simdjson.cpp
    )
    target_include_directories(${target} PUBLIC
      ${CMAKE_CURRENT_BINARY_DIR}/simdjson
    )
  endforeach()
endif()
