project(skyrim_nodejs_plugin LANGUAGES CXX)

if(MSVC)
    find_path(nodejs_INCLUDE_DIR NAMES node.h PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include/node-embedder-api")
    find_library(nodejs_LIBRARY_Debug NAMES node PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib/node-embedder-api")
    find_library(nodejs_LIBRARY_Release NAMES node PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/node-embedder-api")

    file(GLOB_RECURSE src "./*")
    add_library(skyrim_nodejs_plugin SHARED ${src})

    target_link_libraries(skyrim_nodejs_plugin PUBLIC
        "$<IF:$<CONFIG:Debug>,${nodejs_LIBRARY_Debug},${nodejs_LIBRARY_Release}>"
    )

    target_include_directories(skyrim_nodejs_plugin PUBLIC ${nodejs_INCLUDE_DIR})
endif()
