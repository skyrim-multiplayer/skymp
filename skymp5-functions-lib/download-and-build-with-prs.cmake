include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

message(STATUS "Downloading gamemode sources")

# Download the repository using Git

# TODO: Fix CMakeLists.txt: GIT_RESULT/GIT_OUTPUT do not help since configure_file eliminates the variables
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/gamemode-sources)
execute_process(
    COMMAND git clone "${GAMEMODE_REPO_URL}" ${CMAKE_BINARY_DIR}/gamemode-sources
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE GIT_OUTPUT
)

if(GIT_RESULT EQUAL 0)
    message(STATUS "Downloaded gamemode sources")
else()
    message(FATAL_ERROR "Failed to download gamemode sources: ${GIT_OUTPUT}")
endif()

message(STATUS "Downloading Pospelove/auto-merge-action@main (dist/index.js)")

message(STATUS "Downloading ${AUTO_MERGE_REPO}@${AUTO_MERGE_BRANCH} (dist/index.js)")

file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/auto-merge-action)
execute_process(
    COMMAND git clone --branch "${AUTO_MERGE_BRANCH}" "${AUTO_MERGE_REPO_URL}" ${CMAKE_BINARY_DIR}/auto-merge-action
    RESULT_VARIABLE GIT_AM_RESULT
    OUTPUT_VARIABLE GIT_AM_OUTPUT
)

if(GIT_AM_RESULT EQUAL 0)
    message(STATUS "Downloaded ${AUTO_MERGE_REPO}@${AUTO_MERGE_BRANCH} (dist/index.js)")
else()
    message(FATAL_ERROR "Failed to download ${AUTO_MERGE_REPO}@${AUTO_MERGE_BRANCH}: ${GIT_AM_OUTPUT}")
endif()

message(STATUS "Run Pospelove/auto-merge-action@main (dist/index.js)")

# Execute the NodeJS script
set(ENV{INPUT_REPOSITORIES} "${ENV_INPUT_REPOSITORIES}")
set(ENV{INPUT_PATH} ${CMAKE_BINARY_DIR}/gamemode-sources)
execute_process(
    COMMAND node ${CMAKE_BINARY_DIR}/auto-merge-action/dist/index.js
    RESULT_VARIABLE NODE_RESULT
    #OUTPUT_VARIABLE NODE_OUTPUT
)

if(NODE_RESULT EQUAL 0)
    message(STATUS "Successfully ran Pospelove/auto-merge-action@main")
else()
    message(FATAL_ERROR "Failed to run Pospelove/auto-merge-action@main: ${NODE_OUTPUT}")
endif()

message(STATUS "Installing yarn dependencies for gamemode")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/gamemode-sources
    COMMAND install
)

message(STATUS "Installed yarn dependencies for gamemode")

message(STATUS "Building gamemode.js")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/gamemode-sources
    COMMAND build
)

message(STATUS "Built gamemode.js")

message(STATUS "Installing gamemode.js")

file(COPY ${CMAKE_BINARY_DIR}/gamemode-sources/build/gamemode.js DESTINATION "${GAMEMODE_JS_DEST_DIR}")

message(STATUS "Installed gamemode.js")
