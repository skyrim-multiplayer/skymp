include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

message(STATUS "Downloading frontend sources")

# Download the repository using Git

# TODO: Fix CMakeLists.txt: GIT_RESULT/GIT_OUTPUT do not help since configure_file eliminates the variables
file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/frontend-sources)
execute_process(
    COMMAND git clone "${FRONTEND_REPO_URL}" ${CMAKE_BINARY_DIR}/frontend-sources
    RESULT_VARIABLE GIT_RESULT
    OUTPUT_VARIABLE GIT_OUTPUT
)

if(GIT_RESULT EQUAL 0)
    message(STATUS "Downloaded frontend sources")
else()
    message(FATAL_ERROR "Failed to download frontend sources: ${GIT_OUTPUT}")
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
set(ENV{INPUT_PATH} ${CMAKE_BINARY_DIR}/frontend-sources)
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

message(STATUS "Installing yarn dependencies for frontend")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/frontend-sources
    COMMAND install
)

message(STATUS "Installed yarn dependencies for frontend")

message(STATUS "Writing config.js for frontend")

set(FRONT_OUTPUT_PATH "${FRONTEND_JS_DEST_DIR}")
string(REPLACE "\\" "\\\\" FRONT_OUTPUT_PATH ${FRONT_OUTPUT_PATH})
set(FRONT_CFG_PATH "${CMAKE_CURRENT_LIST_DIR}/config.js")
file(WRITE ${FRONT_CFG_PATH} "")
file(APPEND ${FRONT_CFG_PATH} "module.exports = {\n")
file(APPEND ${FRONT_CFG_PATH} "  outputPath:\n")
file(APPEND ${FRONT_CFG_PATH} "    '${FRONT_OUTPUT_PATH}',\n")
file(APPEND ${FRONT_CFG_PATH} "};\n")

message(STATUS "Building frontend")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/frontend-sources
    COMMAND build
)

message(STATUS "Built frontend")
