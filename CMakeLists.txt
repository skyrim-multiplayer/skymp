cmake_minimum_required(VERSION 3.17)
project(skymp5_server)

include(pmm.cmake)
# Clear CMake cache after changing this
# (also for subprojects like platform_se)
set(VCPKG_REVISION cb8a9fe7157f8ad2423742f893955ed9ced05910)
# Download vcpkg dependencies
set(ports
  catch2:x64-windows-static
  sparsepp:x64-windows-static
  nlohmann-json:x64-windows-static
  simdjson:x64-windows-static
  spdlog:x64-windows-static
  slikenet:x64-windows-static
)
foreach(port ${ports})
  message(STATUS "Installing ${port}")
  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES ${port})
endforeach()
if ("${_VCPKG_ROOT_DIR}" STREQUAL "")
  message(FATAL_ERROR "Expected _VCPKG_ROOT_DIR variable to be set by pmm")
endif()

# TODO: Check SHA512
include(cmake_scripts_collection.cmake)

if(WIN32)
  option(SKYRIM_DIR "Path to Skyrim" OFF)
  if (NOT SKYRIM_DIR)
    message(FATAL_ERROR "Please specify SKYRIM_DIR")
  endif()
  if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()

  file(GLOB_RECURSE src "src/client/*" "${CMAKE_CURRENT_SOURCE_DIR}/.clang-format")
  add_library(MpClientPlugin SHARED ${src})
  target_link_libraries(MpClientPlugin PUBLIC mp_common)
  set_target_properties(MpClientPlugin PROPERTIES OUTPUT_NAME "MpClientPlugin")
  apply_default_settings(TARGETS MpClientPlugin)
  list(APPEND VCPKG_DEPENDENT MpClientPlugin)

  add_custom_command(
    TARGET MpClientPlugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MpClientPlugin> ${SKYRIM_DIR}/Data/SKSE/plugins
  )
endif()

set(OPENCPPCOV_DIR "")

file(GLOB_RECURSE src "src/unit/*" "${SKYRIM_MP_ROOT}/.clang-format")
add_executable(unit ${src})
target_link_libraries(unit PRIVATE mp_common)
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/mp_common")
apply_default_settings(TARGETS unit)
list(APPEND VCPKG_DEPENDENT unit)

file(GLOB_RECURSE src "src/server_host/*" "${SKYRIM_MP_ROOT}/.clang-format")
add_executable(server_host ${src})
apply_default_settings(TARGETS server_host)
list(APPEND VCPKG_DEPENDENT server_host)
target_include_directories(server_host PRIVATE "${CMAKE_SOURCE_DIR}/src/third_party")

file(GLOB_RECURSE src "src/server_guest/*" "${SKYRIM_MP_ROOT}/.clang-format")
add_library(server_guest SHARED ${src})
target_link_libraries(server_guest PRIVATE mp_common)
apply_default_settings(TARGETS server_guest)
list(APPEND VCPKG_DEPENDENT server_guest)
target_include_directories(server_guest PRIVATE "${CMAKE_SOURCE_DIR}/src/third_party")

file(GLOB_RECURSE src "src/mp_common/*" "${SKYRIM_MP_ROOT}/.clang-format")
add_library(mp_common STATIC ${src})
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/src/mp_common")
apply_default_settings(TARGETS mp_common)
list(APPEND VCPKG_DEPENDENT mp_common)
if (WIN32)
  target_link_libraries(mp_common PUBLIC ws2_32.lib)
endif()

if (WIN32)
  file(COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/ctest.bat
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/before_ctest.cmake
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )
  file(APPEND ${CMAKE_CURRENT_BINARY_DIR}/before_ctest.cmake
    "\n\n# This file was copied from cmake/before_ctest.cmake, do not modify"
  )
endif()

include(cmake/tests_unit.cmake)

integrate_vcpkg(
  TARGETS ${VCPKG_DEPENDENT}
  VCPKG_PATH "${_VCPKG_ROOT_DIR}"
)
