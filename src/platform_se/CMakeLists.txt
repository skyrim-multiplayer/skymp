cmake_minimum_required(VERSION 3.17)
if ("${CMAKE_PROJECT_NAME}" STREQUAL "")
  project(platform_se)
endif()

if ("${SKYRIM_MP_ROOT}" STREQUAL "")
  message(FATAL_ERROR "SKYRIM_MP_ROOT is not specified")
endif()

set(cmake_dir "${SKYRIM_MP_ROOT}/src/cmake")
if (NOT EXISTS "${cmake_dir}")
  message(FATAL_ERROR "\"${cmake_dir}\" does not exist")
endif()

set(lib_root "${SKYRIM_MP_ROOT}/submodules/CommonLibSSE")
set(third_party "${SKYRIM_MP_ROOT}/src/platform_se/third_party")

file(GLOB_RECURSE lib_src "${lib_root}/src/*" "${lib_root}/include/*")
add_library(CommonLibSSE STATIC ${lib_src})
target_include_directories(CommonLibSSE PUBLIC "${lib_root}/include" "${third_party}")
target_compile_options(CommonLibSSE PUBLIC "/FI\"ForceInclude.h\"" "/FI\"SKSE/Logger.h\"")
target_link_libraries(CommonLibSSE PUBLIC Version)

file(GLOB_RECURSE common_src "${third_party}/common/*")
add_library(common STATIC ${common_src})
target_compile_options(common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(common PRIVATE "${third_party}/common" "${third_party}")

file(GLOB_RECURSE skse64_common_src "${third_party}/skse64_common/*")
add_library(skse64_common STATIC ${skse64_common_src})
target_compile_options(skse64_common PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64_common PRIVATE "${third_party}/common" "${third_party}/skse64_common" "${third_party}")

file(GLOB_RECURSE skse_src "${third_party}/skse64/*")
add_library(skse64 STATIC ${skse_src})
target_compile_options(skse64 PRIVATE "/FI\"IPrefix.h\"")
target_include_directories(skse64 PUBLIC "${third_party}/common" "${third_party}/skse64" "${third_party}")
target_compile_definitions(skse64 PRIVATE RUNTIME=1 RUNTIME_VERSION=0x01050610)
target_link_libraries(skse64 PUBLIC common skse64_common)

include(${cmake_dir}/apply_default_settings.cmake)

if (NOT "${SKIP_SKYRIM_PLATFORM_BUILDING}")
  file(GLOB_RECURSE platform_src "skyrim_platform/*" "${SKYRIM_MP_ROOT}/.clang-format")
  add_library(skyrim_platform SHARED ${platform_src})
  target_link_libraries(skyrim_platform PUBLIC skse64 CommonLibSSE)
  set_target_properties(skyrim_platform PROPERTIES OUTPUT_NAME "SkyrimPlatform")

  apply_default_settings(TARGETS skyrim_platform)

  include(${cmake_dir}/integrate_vcpkg.cmake)
  integrate_vcpkg(
    TARGETS skyrim_platform
    VCPKG_PATH "${VCPKG_ROOT}"
  )

  set_target_properties(skyrim_platform PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
    PDB_OUTPUT_DIRECTORY "bin"
  )

  add_custom_command(
    TARGET skyrim_platform POST_BUILD COMMAND
    ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/touch_$<CONFIG>
  )
endif()

apply_default_settings(TARGETS
  CommonLibSSE common skse64_common skse64
)
