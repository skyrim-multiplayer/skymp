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

    message(STATUS "!!! libnode_LIBRARY_Release: ${libnode_LIBRARY_Release} !!!")

    file(GLOB_RECURSE src "./*")
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

    add_dependencies(skyrim_nodejs_plugin-pack skyrim_nodejs_plugin)

endif()
