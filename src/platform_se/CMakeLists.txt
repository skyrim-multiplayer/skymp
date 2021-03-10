cmake_minimum_required(VERSION 3.19.1)

if ("${CMAKE_PROJECT_NAME}" STREQUAL "" OR "${CMAKE_PROJECT_NAME}" STREQUAL "platform_se")
  set(CMAKE_SYSTEM_VERSION "10.0.17763.0" CACHE STRING INTERNAL FORCE)
  set(CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION "10.0.17763.0" CACHE STRING INTERNAL FORCE)
  project(platform_se)
endif()

if ("${SKYRIM_MP_ROOT}" STREQUAL "")
  message(FATAL_ERROR "SKYRIM_MP_ROOT is not specified")
endif()
set(cmake_dir "${SKYRIM_MP_ROOT}/src/cmake")
if (NOT EXISTS "${cmake_dir}")
  message(FATAL_ERROR "\"${cmake_dir}\" does not exist")
endif()

include(${cmake_dir}/apply_default_settings.cmake)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)
add_library(cef INTERFACE)
target_link_libraries(cef INTERFACE ${CEF_ROOT}/$<CONFIG>/cef_sandbox.lib ${CEF_ROOT}/$<CONFIG>/libcef.lib libcef_dll_wrapper)
target_include_directories(cef INTERFACE ${CEF_ROOT})

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

file(GLOB_RECURSE core_library_src "${third_party}/cef/core_library/*")
add_library(core_library STATIC ${core_library_src})
target_include_directories(core_library PUBLIC "${third_party}/cef/core_library")
list(APPEND VCPKG_DEPENDENT core_library)

file(GLOB_RECURSE reverse_src "${third_party}/cef/reverse/*")
add_library(reverse STATIC ${reverse_src})
target_include_directories(reverse PUBLIC "${third_party}/cef/reverse")
target_link_libraries(reverse PUBLIC core_library)
list(APPEND VCPKG_DEPENDENT reverse)

file(GLOB_RECURSE hooks_src "${third_party}/cef/hooks/*")
add_library(hooks STATIC ${hooks_src})
target_include_directories(hooks PUBLIC "${third_party}/cef/hooks")
target_link_libraries(hooks PUBLIC core_library reverse)
list(APPEND VCPKG_DEPENDENT hooks)

file(GLOB_RECURSE ui_src "${third_party}/cef/ui/*")
add_library(ui STATIC ${ui_src})
target_link_libraries(ui PUBLIC core_library hooks reverse)
target_include_directories(ui PRIVATE ${third_party}/cef/ui )
target_link_libraries(ui PUBLIC "d3d11.lib" "dxguid.lib")
target_link_libraries(ui PUBLIC skyrim_plugin_resources)
target_link_libraries(ui PUBLIC cef)
list(APPEND VCPKG_DEPENDENT ui)

file(GLOB_RECURSE ui_process_src "${third_party}/cef/ui_process/*")
add_executable(SkyrimPlatformCEF ${ui_process_src})
target_link_libraries(SkyrimPlatformCEF PUBLIC core_library)
target_link_libraries(SkyrimPlatformCEF PUBLIC cef)
set_target_properties(SkyrimPlatformCEF PROPERTIES WIN32_EXECUTABLE TRUE)

if (NOT "${SKIP_SKYRIM_PLATFORM_BUILDING}")

  include(${cmake_dir}/CMakeRC.cmake)

  # assets
  set(ASSET_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/skyrim_platform/savefile/template.ess
    ${CMAKE_CURRENT_SOURCE_DIR}/skyrim_platform/assets/cursor.png
  )
  cmrc_add_resource_library(skyrim_plugin_resources ${ASSET_FILES}
    ALIAS assets NAMESPACE skyrim_plugin_resources WHENCE skyrim_platform
  )
  apply_default_settings(TARGETS skyrim_plugin_resources)

  include(${cmake_dir}/npm.cmake)

  set(wdir "${CMAKE_CURRENT_BINARY_DIR}/_codegen")
  file(MAKE_DIRECTORY ${wdir})
  npm_execute_command(
    WORKING_DIRECTORY ${wdir}
    COMMAND init -y
  )
  npm_execute_command(
    WORKING_DIRECTORY ${wdir}
    COMMAND install tsc-watch@4.2.3 typescript@3.8.3 --save
  )

  npm_execute_command(
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/dev_service"
    COMMAND install
  )

  file(GLOB_RECURSE platform_src "skyrim_platform/*" "${SKYRIM_MP_ROOT}/.clang-format")
  add_library(skyrim_platform SHARED ${platform_src})
  target_link_libraries(skyrim_platform PUBLIC skse64 CommonLibSSE skyrim_plugin_resources ui)
  set_target_properties(skyrim_platform PROPERTIES OUTPUT_NAME "SkyrimPlatformImpl")
  target_include_directories(skyrim_platform PRIVATE "${third_party}")
  target_link_libraries(skyrim_platform PRIVATE "${third_party}/frida/frida-gum.lib")
  target_link_libraries(skyrim_platform PRIVATE cef)
  apply_default_settings(TARGETS skyrim_platform)
  list(APPEND VCPKG_DEPENDENT skyrim_platform)

  file(GLOB_RECURSE platform_entry_src "skyrim_platform_entry/*")
  list(APPEND platform_entry_src "${SKYRIM_MP_ROOT}/.clang-format")
  add_library(skyrim_platform_entry SHARED ${platform_entry_src})
  set_target_properties(skyrim_platform_entry PROPERTIES OUTPUT_NAME "SkyrimPlatform")
  target_include_directories(skyrim_platform_entry PRIVATE "${third_party}")
  apply_default_settings(TARGETS skyrim_platform_entry)

  set_target_properties(skyrim_platform SkyrimPlatformCEF skyrim_platform_entry PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "bin"
    PDB_OUTPUT_DIRECTORY "bin"
  )

  function(add_papyrus_library)
    set(options)
    set(oneValueArgs NAME DIRECTORY)
    set(multiValueArgs)
    cmake_parse_arguments(A
      "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    file(GLOB src ${A_DIRECTORY}/*.psc)

    set(OUTPUT_DIR ${CMAKE_SOURCE_DIR}/pex)
    if(SKYRIM_DIR)
      add_custom_target(${A_NAME} ALL
        COMMAND "${SKYRIM_DIR}\\Papyrus Compiler\\PapyrusCompiler.exe"
        ${A_DIRECTORY} -output=${OUTPUT_DIR} -import=${A_DIRECTORY} -all
        SOURCES ${src}
      )
    endif()
  endfunction()

  add_papyrus_library(NAME TESModPlatformPsc DIRECTORY "${CMAKE_SOURCE_DIR}/psc")
  if(TARGET TESModPlatformPsc)
    add_dependencies(TESModPlatformPsc skyrim_platform SkyrimPlatformCEF skyrim_platform_entry)
  endif()

  # For dev_service
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cef_dir.txt ${CEF_ROOT})
  add_custom_target(RestartGame ALL COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/touch_$<CONFIG>)

  add_custom_target(codegen ALL
    COMMAND node ${CMAKE_SOURCE_DIR}/src/platform_se/codegen/index.js
    WORKING_DIRECTORY ${wdir}
  )
endif()

apply_default_settings(TARGETS
  CommonLibSSE common skse64_common skse64
  core_library hooks reverse ui SkyrimPlatformCEF
)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/src/cmake/modules)

foreach(target ${VCPKG_DEPENDENT})
  find_package(ChakraCore REQUIRED)
  target_link_libraries(${target} PUBLIC ChakraCore::ChakraCore)

  find_path(JSON_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
  get_filename_component(JSON_INCLUDE_DIR ${JSON_INCLUDE_DIR} DIRECTORY)
  target_include_directories(${target} PUBLIC ${JSON_INCLUDE_DIR})

  find_path(HTTPLIB_INCLUDE_DIR NAMES httplib.h PATH_SUFFIXES include)
  get_filename_component(HTTPLIB_INCLUDE_DIR ${HTTPLIB_INCLUDE_DIR} DIRECTORY)
  target_include_directories(${target} PUBLIC ${HTTPLIB_INCLUDE_DIR})

  find_package(ZLIB REQUIRED)
  target_link_libraries(${target} PUBLIC ZLIB::ZLIB)

  find_library(MHOOH_LIBRARIES mhook)
  find_path(MHOOH_INCLUDE_DIR NAMES mhook.h PATH_SUFFIXES mhook-lib)
  target_link_libraries(${target} PUBLIC ${MHOOH_LIBRARIES})
  target_include_directories(${target} PUBLIC ${MHOOH_INCLUDE_DIR})

  find_package(directxtk CONFIG REQUIRED)
  target_link_libraries(${target} PUBLIC Microsoft::DirectXTK)
endforeach()
