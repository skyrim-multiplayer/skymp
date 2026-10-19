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

message(STATUS "Installing npm dependencies for frontend")

execute_process(
    COMMAND npm ci
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/frontend-sources
    RESULT_VARIABLE NPM_INSTALL_RESULT
    OUTPUT_VARIABLE NPM_INSTALL_OUTPUT
    ERROR_VARIABLE NPM_INSTALL_ERROR
)

if(NPM_INSTALL_RESULT EQUAL 0)
    message(STATUS "Installed npm dependencies for frontend")
else()
    message(FATAL_ERROR "Failed to install frontend dependencies via npm ci: ${NPM_INSTALL_OUTPUT}\n${NPM_INSTALL_ERROR}")
endif()

message(STATUS "Writing config.js for frontend")

file(WRITE "${CMAKE_BINARY_DIR}/frontend-sources/config.js" "")
file(APPEND "${CMAKE_BINARY_DIR}/frontend-sources/config.js" "module.exports = {\n")
file(APPEND "${CMAKE_BINARY_DIR}/frontend-sources/config.js" "  outputPath:\n")
file(APPEND "${CMAKE_BINARY_DIR}/frontend-sources/config.js" "    '${FRONTEND_JS_DEST_DIR}',\n")
file(APPEND "${CMAKE_BINARY_DIR}/frontend-sources/config.js" "};\n")

message(STATUS "Building frontend")

execute_process(
    COMMAND npm run build
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/frontend-sources
    RESULT_VARIABLE NPM_BUILD_RESULT
    OUTPUT_VARIABLE NPM_BUILD_OUTPUT
    ERROR_VARIABLE NPM_BUILD_ERROR
)

if(NPM_BUILD_RESULT EQUAL 0)
    message(STATUS "Built frontend")
else()
    message(FATAL_ERROR "Failed to build frontend via npm run build: ${NPM_BUILD_OUTPUT}\n${NPM_BUILD_ERROR}")
endif()
