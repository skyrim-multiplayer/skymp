project(skyrim_nodejs_plugin LANGUAGES CXX)

if(MSVC)
    file(GLOB_RECURSE installed "${CMAKE_BINARY_DIR}/vcpkg_installed/*")
    foreach(v ${installed})
        message(STATUS ${v})
    endforeach()
    message(FATAL_ERROR "FOTAL IRAR")

    find_path(nodejs_INCLUDE_DIR NAMES node.h PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/include/node-embedder-api")
    find_library(nodejs_LIBRARY_Debug NAMES node PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/debug/lib/node-embedder-api")
    find_library(nodejs_LIBRARY_Release NAMES node PATHS "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/lib/node-embedder-api")

    target_link_libraries(skyrim_nodejs_plugin PUBLIC
        "$<IF:$<CONFIG:Debug>,${nodejs_LIBRARY_Debug},${nodejs_LIBRARY_Release}>"
    )

    target_include_directories(skyrim_nodejs_plugin PUBLIC ${nodejs_INCLUDE_DIR})

    file(GLOB_RECURSE src "./*")
    add_library(skyrim_nodejs_plugin SHARED ${src})
endif()
