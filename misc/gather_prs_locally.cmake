# A utility script for gathering PRs locally.
# Usage:
# cmake -DGITHUB_TOKEN=<your_token> -P misc/gather_prs_locally.cmake

set(AUTO_MERGE_REPO "Pospelove/auto-merge-action")
set(AUTO_MERGE_BRANCH "main")
set(AUTO_MERGE_REPO_URL "https://github.com/${AUTO_MERGE_REPO}.git")


set(PSEUDO_BINARY_DIR "./build")


include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

message(STATUS "Downloading Pospelove/auto-merge-action@main (dist/index.js)")

message(STATUS "Downloading ${AUTO_MERGE_REPO}@${AUTO_MERGE_BRANCH} (dist/index.js)")

file(REMOVE_RECURSE ${PSEUDO_BINARY_DIR}/auto-merge-action)
execute_process(
    COMMAND git clone --branch "${AUTO_MERGE_BRANCH}" "${AUTO_MERGE_REPO_URL}" ${PSEUDO_BINARY_DIR}/auto-merge-action
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

if("${GITHUB_TOKEN}" STREQUAL "")
    message(FATAL_ERROR "GITHUB_TOKEN is not set")
endif()

set(ENV_INPUT_REPOSITORIES "
[
    {
        \"owner\": \"skyrim-multiplayer\",
        \"repo\": \"skymp\",
        \"labels\": [\"merge-to:indev\"]
    },
    {
        \"owner\": \"skyrim-multiplayer\",
        \"repo\": \"skymp5-patches\",
        \"labels\": [\"merge-to:indev\"],
        \"token\": \"${GITHUB_TOKEN}\"
    }
]
")

set(ENV{INPUT_REPOSITORIES} "${ENV_INPUT_REPOSITORIES}")
set(ENV{INPUT_PATH} ${CMAKE_SOURCE_DIR})
execute_process(
    COMMAND node ${PSEUDO_BINARY_DIR}/auto-merge-action/dist/index.js
    RESULT_VARIABLE NODE_RESULT
    OUTPUT_VARIABLE NODE_OUTPUT
)

if(NODE_RESULT EQUAL 0)
    message(STATUS "Successfully ran Pospelove/auto-merge-action@main")
else()
    message(FATAL_ERROR "Failed to run Pospelove/auto-merge-action@main: ${NODE_OUTPUT}")
endif()

# Commit the changes locally
# git add . && git commit -m "Gather PRs"

message(STATUS "Commit the changes locally")

execute_process(
    COMMAND git add .
    RESULT_VARIABLE GIT_ADD_RESULT
    OUTPUT_VARIABLE GIT_ADD_OUTPUT
)

if(GIT_ADD_RESULT EQUAL 0)
    message(STATUS "Added changes to the index")
else()
    message(FATAL_ERROR "Failed to add changes to the index: ${GIT_ADD_OUTPUT}")
endif()

execute_process(
    COMMAND git commit -m "Gather PRs"
    RESULT_VARIABLE GIT_COMMIT_RESULT
    OUTPUT_VARIABLE GIT_COMMIT_OUTPUT
)

if(GIT_COMMIT_RESULT EQUAL 0)
    message(STATUS "Committed the changes")
else()
    message(FATAL_ERROR "Failed to commit the changes: ${GIT_COMMIT_OUTPUT}")
endif()
