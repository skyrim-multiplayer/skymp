project(skyrim_nodejs_plugin LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

if(MSVC AND BUILD_NODEJS_PLUGIN)
    file(GLOB_RECURSE installed "${CMAKE_BINARY_DIR}/vcpkg_installed/*")
    message(STATUS "!!! PRINTING INSTALLED !!!")
    foreach(v ${installed})
        message(STATUS ${v})
    endforeach()
    message(STATUS "!!! FINISHED PRINTING INSTALLED !!!")

    add_library(node-embedder-api INTERFACE)
    find_path(node_INCLUDE_DIR NAMES js_native_api.h PATH_SUFFIXES node-embedder-api)
    find_library(libnode_LIBRARY_Debug NAMES libnode PATH_SUFFIXES node-embedder-api)
    string(REPLACE "/debug/lib/" "/lib/" libnode_LIBRARY_Release "${libnode_LIBRARY_Debug}")
    target_link_libraries(node-embedder-api INTERFACE "$<IF:$<CONFIG:Debug>,${libnode_LIBRARY_Debug},${libnode_LIBRARY_Release}>")
    target_include_directories(node-embedder-api INTERFACE ${node_INCLUDE_DIR})

    # also find dll for each configuration
    find_path(node_dll_debug NAMES libnode.dll PATH_SUFFIXES debug/bin/node-embedder-api)
    if(node_dll_debug AND IS_DIRECTORY ${node_dll_debug})
        set(node_dll_debug "${node_dll_debug}/libnode.dll")
    endif()
    find_path(node_dll_release NAMES libnode.dll PATH_SUFFIXES bin/node-embedder-api)
    if(node_dll_release AND IS_DIRECTORY ${node_dll_release})
        set(node_dll_release "${node_dll_release}/libnode.dll")
    endif()

    # if not found debug, use release for debug as well
    if(NOT node_dll_debug)
        set(node_dll_debug ${node_dll_release})
    endif()

    message(STATUS "!!! node_dll_debug: ${node_dll_debug} !!!")
    message(STATUS "!!! node_dll_release: ${node_dll_release} !!!")

    message(STATUS "!!! libnode_LIBRARY_Release: ${libnode_LIBRARY_Release} !!!")

    file(GLOB_RECURSE src "./*")
    list(FILTER src EXCLUDE REGEX ".*test.*")
    add_library(skyrim_nodejs_plugin SHARED ${src})
    target_link_libraries(skyrim_nodejs_plugin PUBLIC node-embedder-api)

    set_target_properties(skyrim_nodejs_plugin PROPERTIES OUTPUT_NAME SkyrimNodeJS)

    target_compile_definitions(skyrim_nodejs_plugin PRIVATE BUILDING_NODE_EXTENSION)

    if(SKYRIM_SE)
        find_package(commonlibse REQUIRED CONFIGS CommonLibSSEConfig.cmake)
        target_link_libraries(skyrim_nodejs_plugin PRIVATE CommonLibSSE::CommonLibSSE)
        target_compile_definitions(skyrim_nodejs_plugin PRIVATE SKYRIMSE)
    else()
        find_package(commonlibae REQUIRED CONFIGS CommonLibSSEConfig.cmake)
        target_link_libraries(skyrim_nodejs_plugin PRIVATE CommonLibSSE::CommonLibSSE)
    endif()

    apply_default_settings(TARGETS skyrim_nodejs_plugin)

    set(SKSE_PLUGINS_DIR "${CMAKE_BINARY_DIR}/dist/client/Data/SKSE/Plugins")

    add_custom_target(skyrim_nodejs_plugin-pack ALL
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:skyrim_nodejs_plugin> "${SKSE_PLUGINS_DIR}"
    )

    # install libnode.dll to a directory of the plugin compiled

    add_custom_command(TARGET skyrim_nodejs_plugin-pack POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "$<IF:$<CONFIG:Debug>,${node_dll_debug},${node_dll_release}>" $<TARGET_FILE_DIR:skyrim_nodejs_plugin>
    )

    add_dependencies(skyrim_nodejs_plugin-pack skyrim_nodejs_plugin)

    add_executable(skyrim_nodejs_plugin_test "test/main.cpp")
endif()
