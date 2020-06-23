include(${CMAKE_SOURCE_DIR}/cmake/third_party/pmm.cmake)
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

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/unit/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_executable(unit ${src})
target_link_libraries(unit PRIVATE server_guest_lib)
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mp_common")
apply_default_settings(TARGETS unit)
list(APPEND VCPKG_DEPENDENT unit)

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/mp_common/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(mp_common STATIC ${src})
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/mp_common")
apply_default_settings(TARGETS mp_common)
list(APPEND VCPKG_DEPENDENT mp_common)
if (WIN32)
  target_link_libraries(mp_common PUBLIC ws2_32.lib)
endif()

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/server_guest_lib/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(server_guest_lib STATIC ${src})
target_include_directories(server_guest_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/server_guest_lib")
target_include_directories(server_guest_lib PRIVATE "${CMAKE_SOURCE_DIR}/third_party")
apply_default_settings(TARGETS server_guest_lib)
list(APPEND VCPKG_DEPENDENT server_guest_lib)
target_link_libraries(server_guest_lib PUBLIC mp_common)
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

include(${CMAKE_SOURCE_DIR}/cmake/tests_unit.cmake)

integrate_vcpkg(
  TARGETS ${VCPKG_DEPENDENT}
  VCPKG_PATH "${_VCPKG_ROOT_DIR}"
)
