project(platform_se VERSION 2.7.1 LANGUAGES CXX)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)

if(MSVC)
  include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library_ck.cmake)

  add_library(cef INTERFACE)
  find_library(CEF_LIBRARY_DEBUG libcef_dll_wrapper)
  string(REPLACE "/debug/lib/" "/lib/" CEF_LIBRARY_RELEASE ${CEF_LIBRARY_DEBUG})
  find_path(CEF_INCLUDE_DIR include/cef_app.h)
  find_path(CEF_SHARE share/cef-prebuilt)
  set(CEF_SHARE "${CEF_SHARE}/share/cef-prebuilt")
  target_link_libraries(cef INTERFACE "$<IF:$<CONFIG:Debug>,${CEF_LIBRARY_DEBUG},${CEF_LIBRARY_RELEASE}>")
  target_link_libraries(cef INTERFACE "${CEF_SHARE}/$<CONFIG>/cef_sandbox.lib")
  target_link_libraries(cef INTERFACE "${CEF_SHARE}/$<CONFIG>/libcef.lib")
  target_include_directories(cef INTERFACE ${CEF_INCLUDE_DIR})

  set(third_party "${SKYRIM_PLATFORM_SOURCE_DIR}/src/third_party")
  set(tilted "${SKYRIM_PLATFORM_SOURCE_DIR}/src/tilted")

  file(GLOB_RECURSE core_library_src "${tilted}/core_library/*")
  add_library(core_library STATIC ${core_library_src})
  target_include_directories(core_library PUBLIC "${tilted}/core_library")
  list(APPEND VCPKG_DEPENDENT core_library)

  file(GLOB_RECURSE reverse_src "${tilted}/reverse/*")
  add_library(reverse STATIC ${reverse_src})
  target_include_directories(reverse PUBLIC "${tilted}/reverse")
  target_link_libraries(reverse PUBLIC core_library)
  list(APPEND VCPKG_DEPENDENT reverse)

  file(GLOB_RECURSE hooks_src "${tilted}/hooks/*")
  add_library(hooks STATIC ${hooks_src})
  target_include_directories(hooks PUBLIC "${tilted}/hooks")
  target_link_libraries(hooks PUBLIC core_library reverse)
  list(APPEND VCPKG_DEPENDENT hooks)

  file(GLOB_RECURSE ui_src "${tilted}/ui/*")
  add_library(ui STATIC ${ui_src})
  target_link_libraries(ui PUBLIC core_library hooks reverse)
  target_include_directories(ui PRIVATE ${tilted}/ui )
  target_link_libraries(ui PUBLIC "d3d11.lib" "dxguid.lib")
  target_link_libraries(ui PUBLIC skyrim_plugin_resources)
  target_link_libraries(ui PUBLIC cef)
  list(APPEND VCPKG_DEPENDENT ui)

  file(GLOB_RECURSE ui_process_src "${tilted}/ui_process/*")
  add_executable(SkyrimPlatformCEF ${ui_process_src})
  target_link_libraries(SkyrimPlatformCEF PUBLIC core_library)
  target_link_libraries(SkyrimPlatformCEF PUBLIC cef)
  # suppress missing pdb warning for cef
  target_link_options(SkyrimPlatformCEF PRIVATE "LINKER:-ignore:4099")
  set_target_properties(SkyrimPlatformCEF PROPERTIES WIN32_EXECUTABLE TRUE)
  set_target_properties(SkyrimPlatformCEF PROPERTIES SUFFIX ".exe.hidden")
endif()

if(NOT "${SKIP_SKYRIM_PLATFORM_BUILDING}")

  find_package(CMakeRC CONFIG REQUIRED)

  # assets
  set(ASSET_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/skyrim_platform/assets/template.ess
    ${CMAKE_CURRENT_SOURCE_DIR}/skyrim_platform/assets/cursor.png
  )
  cmrc_add_resource_library(skyrim_plugin_resources ${ASSET_FILES}
    ALIAS assets NAMESPACE skyrim_plugin_resources WHENCE skyrim_platform
  )
  apply_default_settings(TARGETS skyrim_plugin_resources)

  include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)
  yarn_execute_command(
    WORKING_DIRECTORY "${SKYRIM_PLATFORM_SOURCE_DIR}/tools/dev_service"
    COMMAND install
  )

  if(MSVC)
    if(SKYRIM_SE)
      add_compile_definitions(SKYRIMSE)
    endif()

    file(GLOB_RECURSE platform_src "skyrim_platform/*")
    list(APPEND platform_src "${CMAKE_SOURCE_DIR}/.clang-format")
    list(APPEND platform_src ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h)
    list(APPEND platform_src ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    add_library(skyrim_platform SHARED ${platform_src})
    set_target_properties(skyrim_platform PROPERTIES OUTPUT_NAME "SkyrimPlatformImpl")
    target_include_directories(skyrim_platform PRIVATE "${tilted}")
    target_include_directories(skyrim_platform PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
    target_link_libraries(skyrim_platform PRIVATE papyrus-vm-lib savefile skyrim_plugin_resources ui cef platform_lib viet)
    
    find_path(FRIDA_GUM_INCLUDE_DIR NAMES frida-gum.h)
    find_library(FRIDA_GUM_LIBRARY NAMES frida-gum.lib)

    target_include_directories(skyrim_platform PRIVATE ${FRIDA_GUM_INCLUDE_DIR})
    target_link_libraries(skyrim_platform PRIVATE ${FRIDA_GUM_LIBRARY})

    # suppress missing pdb warning for cef and frida
    target_link_options(skyrim_platform PRIVATE "LINKER:-ignore:4099")
    target_precompile_headers(skyrim_platform	PRIVATE	skyrim_platform/PCH.h)
    apply_default_settings(TARGETS skyrim_platform)
    list(APPEND VCPKG_DEPENDENT skyrim_platform)

    file(GLOB_RECURSE platform_entry_src "skyrim_platform_entry/*")
    list(APPEND platform_entry_src "${CMAKE_SOURCE_DIR}/.clang-format")
    list(APPEND platform_entry_src ${CMAKE_CURRENT_BINARY_DIR}/include/Version.h)
    list(APPEND platform_entry_src ${CMAKE_CURRENT_BINARY_DIR}/version.rc)
    add_library(skyrim_platform_entry SHARED ${platform_entry_src})
    set_target_properties(skyrim_platform_entry PROPERTIES OUTPUT_NAME "SkyrimPlatform")
    target_include_directories(skyrim_platform_entry PRIVATE "${third_party}")
    target_include_directories(skyrim_platform_entry PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include)
    target_link_libraries(skyrim_platform_entry PRIVATE CommonLibSSE::CommonLibSSE)
    target_precompile_headers(skyrim_platform_entry	PRIVATE	skyrim_platform_entry/PCH.h)
    apply_default_settings(TARGETS skyrim_platform_entry)

    set_target_properties(skyrim_platform SkyrimPlatformCEF skyrim_platform_entry PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "bin"
      PDB_OUTPUT_DIRECTORY "bin"
    )

    set(DEPENDENCIES_FOR_CUSTOM_TARGETS skyrim_platform SkyrimPlatformCEF skyrim_platform_entry)

  add_papyrus_library_ck(
    NAME TESModPlatformPsc
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/psc"
    OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/pex"
    COMPILER_EXECUTABLE_PATH "${SKYRIM_DIR}/Papyrus compiler/PapyrusCompiler.exe"
  )
  if(TARGET TESModPlatformPsc)
    add_dependencies(TESModPlatformPsc ${DEPENDENCIES_FOR_CUSTOM_TARGETS})
    list(APPEND DEPENDENCIES_FOR_CUSTOM_TARGETS TESModPlatformPsc)
  endif()

    file(GLOB papyrus_sources "${CMAKE_CURRENT_SOURCE_DIR}/psc/*.psc")
    set(def "")
    foreach(source_path ${papyrus_sources})
      get_filename_component(source_path ${source_path} NAME)
      string(APPEND def "${source_path} ")
    endforeach()
    target_compile_definitions(skyrim_platform PUBLIC PAPYRUS_SOURCES=\"${def}\")

    # cef_dir.txt is used by dev_service in any mode
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/cef_dir.txt "${CEF_SHARE}")

    # touch dev_service in watch mode
    add_custom_target(RestartGame ALL COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/touch_$<CONFIG>)
    add_dependencies(RestartGame ${DEPENDENCIES_FOR_CUSTOM_TARGETS})
  endif()

  add_executable(TSConverter ${CMAKE_CURRENT_LIST_DIR}/codegen/TSConverter.cpp)
  apply_default_settings(TARGETS TSConverter)
  set_target_properties(TSConverter PROPERTIES CXX_STANDARD 20)
  list(APPEND VCPKG_DEPENDENT TSConverter)

  set(CONVERT_FILES_DIR ${CMAKE_CURRENT_LIST_DIR}/codegen/convert-files/)
  list(APPEND CODEGEN_ARGS
    "${CONVERT_FILES_DIR}/FunctionsDump.txt"
    "${CONVERT_FILES_DIR}/Definitions.txt"
    "${CMAKE_CURRENT_BINARY_DIR}/_codegen/skyrimPlatform.ts"
  )
  add_custom_target(codegen ALL COMMAND $<TARGET_FILE:TSConverter> ${CODEGEN_ARGS})
  add_dependencies(codegen TSConverter)

  # force "skyrim-platform" custom target to be built strictly after codegen
  list(APPEND DEPENDENCIES_FOR_CUSTOM_TARGETS codegen)

  # this target was originally called pack, now it is called skyrim-platform
  # so we can use add_dependencies(x skyrim-platform) in other projects
  add_custom_target(skyrim-platform ALL
    COMMAND yarn --cwd \"${SKYRIM_PLATFORM_SOURCE_DIR}/tools/dev_service\" run pack$<$<CONFIG:Debug>:-debug>
  )
  add_dependencies(skyrim-platform ${DEPENDENCIES_FOR_CUSTOM_TARGETS})
endif()

if(MSVC)
  apply_default_settings(TARGETS
    core_library hooks reverse ui SkyrimPlatformCEF
  )
endif()

link_vcpkg_dependencies(TARGETS ${VCPKG_DEPENDENT})
