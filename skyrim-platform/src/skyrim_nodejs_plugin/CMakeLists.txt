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
    find_library(node_LIBRARY_Debug NAMES node PATH_SUFFIXES node-embedder-api)
    string(REPLACE "/debug/lib/" "/lib/" node_LIBRARY_Release "${node_LIBRARY_Debug}")
    target_link_libraries(node-embedder-api INTERFACE "$<IF:$<CONFIG:Debug>,${node_LIBRARY_Debug},${node_LIBRARY_Release}>")
    target_include_directories(node-embedder-api INTERFACE ${node_INCLUDE_DIR})

    # determine parent dir of lib and debug lib
    get_filename_component(node_LIB_DIR_Debug ${node_LIBRARY_Debug} DIRECTORY)
    get_filename_component(node_LIB_DIR_Release ${node_LIBRARY_Release} DIRECTORY)

    # glob .lib files in lib and debug lib
    file(GLOB_RECURSE node_additional_libs_debug "${node_LIB_DIR_Debug}/*.lib")
    file(GLOB_RECURSE node_additional_libs_release "${node_LIB_DIR_Release}/*.lib")

    # filter out mksnapshot.lib | node.lib
    list(FILTER node_additional_libs_debug EXCLUDE REGEX "mksnapshot.lib|node.lib")
    list(FILTER node_additional_libs_release EXCLUDE REGEX "mksnapshot.lib|node.lib")

    # link them to node-embedder-api interface target
    foreach(path ${node_additional_libs_debug})
        target_link_libraries(node-embedder-api INTERFACE "$<IF:$<CONFIG:Debug>,${path},>")
    endforeach()
    foreach(path ${node_additional_libs_release})
        target_link_libraries(node-embedder-api INTERFACE "$<IF:$<CONFIG:Release>,${path},>")
    endforeach()

    file(GLOB_RECURSE src "./*")
    add_library(skyrim_nodejs_plugin SHARED ${src})
    target_link_libraries(skyrim_nodejs_plugin PUBLIC node-embedder-api)

    set_target_properties(skyrim_nodejs_plugin PROPERTIES OUTPUT_NAME SkyrimNodeJS)

    target_link_libraries(skyrim_nodejs_plugin PRIVATE ws2_32.lib)
    target_link_libraries(skyrim_nodejs_plugin PRIVATE Iphlpapi.lib)
    target_link_libraries(skyrim_nodejs_plugin PRIVATE Userenv.lib)
    target_link_libraries(skyrim_nodejs_plugin PRIVATE Winmm.lib)

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
