# At the moment of writing this gives ~10% speedup of cmake command on Windows
# https://gitlab.kitware.com/cmake/cmake/-/issues/23154
if(POLICY 0115)
  cmake_policy(SET CMP0115 NEW)
endif()

if("${VCPKG_TARGET_TRIPLET}" STREQUAL "")
  if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-sp")
  elseif(APPLE)
    set(VCPKG_TARGET_TRIPLET "x64-osx")
  else()
    set(VCPKG_TARGET_TRIPLET "x64-linux")
  endif()
endif()

set(VCPKG_OVERLAY_TRIPLETS "${CMAKE_CURRENT_LIST_DIR}/overlay_triplets")
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_LIST_DIR}/overlay_ports")

option(NO_CLEAN_AFTER_BUILD "Don't clean vcpkg temporary folders after build" OFF)

if("$ENV{CI}" STREQUAL "true" OR NO_CLEAN_AFTER_BUILD)
  set(VCPKG_INSTALL_OPTIONS --no-print-usage)
else()
  set(VCPKG_INSTALL_OPTIONS --no-print-usage --clean-after-build)
endif()

if("$ENV{CI}" STREQUAL "true" AND WIN32)
  # The same submodule but moved to a larger disk in Windows CI. See action files:
  # - https://github.com/skyrim-multiplayer/skymp/blob/main/.github/workflows/pr-windows.yml
  # - https://github.com/skyrim-multiplayer/skymp/blob/main/.github/workflows/pr-windows-ae.yml
  set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
else()
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
endif()

# File from vcpkg submodule. This indicates inability to find this file or checkout submodules.
if(NOT EXISTS "${CMAKE_TOOLCHAIN_FILE}")
  set(msg "${CMAKE_TOOLCHAIN_FILE} doesn't exist. It seems that vcpkg submodule is not initialized.")
  set(msg "${msg}\nUse commands below to initialize:")
  set(msg "${msg}\n  git submodule init")
  set(msg "${msg}\n  git submodule update")
  message(FATAL_ERROR "${msg}")
endif()

if(NOT "${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}/build")
  set(msg "CMake binary directory ${CMAKE_BINARY_DIR} must be <repo_root>/build (${CMAKE_SOURCE_DIR}/build)")
  set(msg "${msg}\nUse commands below after cloning the repo (replace path with your actual Skyrim SE folder):")
  set(msg "${msg}\n  mkdir build")
  set(msg "${msg}\n  cd build")
  set(msg "${msg}\nFor users who have Skyrim SE installed:")
  set(msg "${msg}\n  cmake .. -DSKYRIM_DIR=\"C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition\"")
  set(msg "${msg}\nFor users who don't have Skyrim SE installed:")
  set(msg "${msg}\n  cmake ..")
  message(FATAL_ERROR "${msg}")
endif()

# Bool options (Part 1)
option(SKYRIM_SE "Legacy Skyrim SE (1.5) build" OFF)
option(BUILD_NODEJS "Build NodeJS port if ON, use a pre-built otherwise" OFF)

if(SKYRIM_SE)
  list(APPEND VCPKG_MANIFEST_FEATURES "skyrim-se")
else()
  list(APPEND VCPKG_MANIFEST_FEATURES "skyrim-ae")
endif()

if(BUILD_NODEJS)
  list(APPEND VCPKG_MANIFEST_FEATURES "build-nodejs")
else()
  list(APPEND VCPKG_MANIFEST_FEATURES "prebuilt-nodejs")
endif()

cmake_minimum_required(VERSION 3.19)
project(skymp)
enable_testing()

# Check the Visual Studio version
if(MSVC)
  if(NOT CMAKE_GENERATOR MATCHES "Visual Studio 17 2022")
    message(FATAL_ERROR "This project requires \"Visual Studio 17 2022\". But the generator was \"${CMAKE_GENERATOR}\"")
  endif()
endif()

# Bool options (Part 2)
option(JS_ENGINE_TRACING_ENABLED "Useful for JsEngine debugging. See JsEngine.h" OFF)
option(PREPARE_NEXUS_ARCHIVES "Prepare SP and other archives during build or not" OFF)
option(BUILD_UNIT_TESTS "Build unit tests (excluded from build when off - workaround for #1182)" ON)
option(INSTALL_CLIENT_DIST "Install the client into SKYRIM_DIR after build" OFF)
option(BUILD_GAMEMODE "Build gamemode" OFF)
option(OFFLINE_MODE "Enable offline mode in generated server settings and client settings" ON)

message(STATUS JS_ENGINE_TRACING_ENABLED=${JS_ENGINE_TRACING_ENABLED})
message(STATUS PREPARE_NEXUS_ARCHIVES=${PREPARE_NEXUS_ARCHIVES})
message(STATUS SKYRIM_SE=${SKYRIM_SE})
message(STATUS BUILD_UNIT_TESTS=${BUILD_UNIT_TESTS})
message(STATUS INSTALL_CLIENT_DIST=${INSTALL_CLIENT_DIST})
message(STATUS BUILD_GAMEMODE=${BUILD_GAMEMODE})
message(STATUS OFFLINE_MODE=${OFFLINE_MODE})
message(STATUS BUILD_NODEJS=${BUILD_NODEJS})

if(PREPARE_NEXUS_ARCHIVES AND EMSCRIPTEN)
  message(FATAL_ERROR "PREPARE_NEXUS_ARCHIVES is not supported on Emscripten")
endif()

# Note that SkyrimPlatform performance drops significantly with JS_ENGINE_TRACING_ENABLED set to true.

if(JS_ENGINE_TRACING_ENABLED)
  add_compile_definitions(JS_ENGINE_TRACING_ENABLED)
endif()

# Path options
option(UNIT_DATA_DIR "Path to directory with Skyrim.esm and other data files required for testing. CMAKE_SOURCE_DIR would be used to resolve a relative path if passed." OFF)
option(SKYRIM_DIR "Path to Skyrim, would be used for tests if UNIT_DATA_DIR is not specified" OFF)
option(CPPCOV_PATH "Path to OpenCppCoverage" OFF)

if(UNIT_DATA_DIR AND NOT IS_ABSOLUTE "${UNIT_DATA_DIR}")
  get_filename_component(UNIT_DATA_DIR "${UNIT_DATA_DIR}" ABSOLUTE BASE_DIR ${CMAKE_SOURCE_DIR})
endif()

if(NOT SKYRIM_DIR)
  message(STATUS "SKYRIM_DIR is not specified. You will have to fill server config with esm paths manually")
endif()
string(REPLACE "\\" "/" SKYRIM_DIR "${SKYRIM_DIR}")

if(SKYRIM_DIR)
  if (NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()
endif()

if(NOT CPPCOV_PATH)
  message(STATUS "CPPCOV_PATH is not specified, coverage will not be calculated")
endif()
string(REPLACE "\\" "/" CPPCOV_PATH "${CPPCOV_PATH}")

if(CPPCOV_PATH)
  if (NOT EXISTS ${CPPCOV_PATH}/OpenCppCoverage.exe)
    message(FATAL_ERROR "Bad CPPCOV_PATH: ${CPPCOV_PATH}")
  endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

# Initialize the CMAKEPROJ projects and their configurations
list(APPEND CMAKEPROJ_PROJECTS_LIST)  # List of all projects
set(CMAKEPROJ_PROJECTS_DEPENDENCIES)  # Dependencies for each project
set(CMAKEPROJ_PROJECTS_PRIORITY)      # Priority for each project

# Glob directories under source directory root
file(GLOB ROOT_DIRS RELATIVE ${CMAKE_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/*)
foreach(dir ${ROOT_DIRS})
  if(IS_DIRECTORY ${CMAKE_SOURCE_DIR}/${dir})
    # Check if directory has a cmakeproj.cmake file
    if(EXISTS ${CMAKE_SOURCE_DIR}/${dir}/cmakeproj.cmake)
      include(${CMAKE_SOURCE_DIR}/${dir}/cmakeproj.cmake)
      # Add project to the list if not already added
      if(NOT "${CMAKEPROJ_PROJECTS_LIST}" MATCHES "${dir}")
        list(APPEND CMAKEPROJ_PROJECTS_LIST ${dir})
      endif()
      # Set default priority if not set
      if (NOT DEFINED CMAKEPROJ_PRIORITY_${dir})
        set(CMAKEPROJ_PRIORITY_${dir} 0) # Default priority 0 if not defined
      endif()

      # Store dependencies and priorities for sorting
      set(CMAKEPROJ_PROJECTS_DEPENDENCIES ${CMAKEPROJ_PROJECTS_DEPENDENCIES} "${CMAKEPROJ_DEPS_${dir}}")
      set(CMAKEPROJ_PROJECTS_PRIORITY ${CMAKEPROJ_PROJECTS_PRIORITY} ${CMAKEPROJ_PRIORITY_${dir}})
    endif()
  endif()
endforeach()

foreach(proj ${CMAKEPROJ_PROJECTS_LIST})
  list(APPEND priorities_list ${CMAKEPROJ_PRIORITY_${proj}})  
endforeach()
list(REMOVE_DUPLICATES priorities_list)
list(SORT priorities_list COMPARE NATURAL)

foreach(priority ${priorities_list})
  foreach(proj ${CMAKEPROJ_PROJECTS_LIST})
    if(${CMAKEPROJ_PRIORITY_${proj}} STREQUAL ${priority})
      list(APPEND sorted_projects ${proj})
    endif()
  endforeach()
endforeach()

foreach(project ${sorted_projects})
  message(STATUS "Adding project: ${project} with priority ${CMAKEPROJ_PRIORITY_${project}}")
  add_subdirectory(${project})
endforeach()

if(PREPARE_NEXUS_ARCHIVES)
  add_custom_target(
    prepare_nexus_archives ALL
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/cmake/prepare_nexus_archives.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )

  add_dependencies(prepare_nexus_archives skyrim-platform papyrus-vm)
  set(TARGETS_ADDITIONAL
    skymp5-client
    skymp5-front
    skymp5-functions-lib
    skymp5-scripts
    skymp5-server
  )
  foreach(target ${TARGETS_ADDITIONAL})
    if(TARGET ${target})
      add_dependencies(prepare_nexus_archives ${target})
    endif()
  endforeach()
endif()

# One or more of these targets help dev_service not to crash during build (watch mode). Not sure which target exactly.
set(TARGETS_ADDITIONAL
    skymp5-client
    skymp5-front
    skymp5-functions-lib
    skymp5-scripts
    skymp5-server
  )
foreach(target ${TARGETS_ADDITIONAL})
  if(TARGET ${target} AND TARGET RestartGame)
    add_dependencies(RestartGame ${target})
  endif()
endforeach()

if(INSTALL_CLIENT_DIST)
  add_custom_target(
    install_client_dist ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/dist/client ${SKYRIM_DIR}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  )
  add_dependencies(install_client_dist skymp5-client skyrim-platform)
endif()
