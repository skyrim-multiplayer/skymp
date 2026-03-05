# A utility script for Git Hooks installation
# Usage:
# cmake -P misc/install_git_hooks.cmake

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(REPO_ROOT ${REPO_ROOT} REALPATH)

file(MAKE_DIRECTORY ${REPO_ROOT}/.git/hooks)

# Download the single bundled linter from skyrim-multiplayer/linter
set(LINTER_URL "https://raw.githubusercontent.com/skyrim-multiplayer/linter/2d4a9a96989b066c9cf1fb70ae1169eec106d45e/dist/linter.mjs")
set(LINTER_SHA256 "b3727c7cbf9787f5b7b91bd340b5553d3c1f51cb176dd362d754aa3579667040")
set(LINTER_DIR "${REPO_ROOT}/.linter")
set(LINTER_FILE "${LINTER_DIR}/linter.mjs")

file(MAKE_DIRECTORY ${LINTER_DIR})

message(STATUS "Downloading linter from ${LINTER_URL}")
file(DOWNLOAD ${LINTER_URL} ${LINTER_FILE}
    EXPECTED_HASH SHA256=${LINTER_SHA256}
    STATUS DOWNLOAD_STATUS
)
list(GET DOWNLOAD_STATUS 0 DOWNLOAD_CODE)
if(NOT DOWNLOAD_CODE EQUAL 0)
    list(GET DOWNLOAD_STATUS 1 DOWNLOAD_ERROR)
    message(FATAL_ERROR "Failed to download linter: ${DOWNLOAD_ERROR}")
endif()
message(STATUS "Linter downloaded and SHA256 verified")

# Write pre-commit hook
file(WRITE ${REPO_ROOT}/.git/hooks/pre-commit "#!/bin/sh\nnode .linter/linter.mjs --fix --add --mode hook\n")

# Make pre-commit hook executable if running on Unix
if(UNIX)
    execute_process(COMMAND chmod +x ${REPO_ROOT}/.git/hooks/pre-commit)
endif()

message(STATUS "Git hooks installed successfully")
