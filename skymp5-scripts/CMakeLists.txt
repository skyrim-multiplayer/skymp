project(skymp5-scripts)

include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library_ck.cmake)

if(BUILD_SCRIPTS)
    add_papyrus_library_ck(
        NAME skymp5-scripts
        DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/psc
        OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/pex
        COMPILER_EXECUTABLE_PATH "${SKYRIM_DIR}/Papyrus compiler/PapyrusCompiler.exe"
    )

    add_custom_command(
        TARGET skymp5-scripts POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/pex ${CMAKE_BINARY_DIR}/dist/server/data/scripts
    )
else()
    add_custom_target(skymp5-scripts ALL
        SOURCES ${source}
        COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-scripts is disabled. To enable it, set BUILD_SCRIPTS to ON."
    )
endif()
