project(skyrim_nodejs_plugin LANGUAGES CXX)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

if(MSVC)
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

    file(GLOB_RECURSE src "./*")
    add_library(skyrim_nodejs_plugin SHARED ${src})
    target_link_libraries(skyrim_nodejs_plugin PUBLIC node-embedder-api)

    set_target_properties(skyrim_nodejs_plugin PROPERTIES OUTPUT_NAME SkyrimNodeJS)

    if(SKYRIM_SE)
        find_package(commonlibse REQUIRED CONFIGS CommonLibSSEConfig.cmake)
        target_link_libraries(skyrim_nodejs_plugin PRIVATE CommonLibSSE::CommonLibSSE)
        target_compile_definitions(skyrim_nodejs_plugin PRIVATE SKYRIMSE)
    else()
        find_package(commonlibae REQUIRED CONFIGS CommonLibSSEConfig.cmake)
        target_link_libraries(skyrim_nodejs_plugin PRIVATE CommonLibSSE::CommonLibSSE)
    endif()

    apply_default_settings(TARGETS skyrim_nodejs_plugin)

endif()
