# A utility script for Git Hooks installation
# Usage:
# cmake -P misc/install_git_hooks.cmake

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(REPO_ROOT ${REPO_ROOT} REALPATH)

file(MAKE_DIRECTORY ${REPO_ROOT}/.git/hooks)

# Install linter from skyrim-multiplayer/linter pinned by commit.
set(LINTER_COMMIT "9f6870f600b2dca73b72106f08400fb9d6751d2d")
set(LINTER_PACKAGE "https://github.com/skyrim-multiplayer/linter#${LINTER_COMMIT}")
set(OLD_LINTER_DIR "${REPO_ROOT}/.linter")
set(PRE_COMMIT_HOOK "${REPO_ROOT}/.git/hooks/pre-commit")

# Remove old hook artifacts from the previous local-bundle approach.
if(EXISTS ${OLD_LINTER_DIR})
    file(REMOVE_RECURSE ${OLD_LINTER_DIR})
endif()

if(EXISTS ${PRE_COMMIT_HOOK})
    file(REMOVE ${PRE_COMMIT_HOOK})
endif()

message(STATUS "Installing skymp-linter from ${LINTER_PACKAGE}")
execute_process(
    COMMAND yarn global add ${LINTER_PACKAGE}
    RESULT_VARIABLE YARN_ADD_RESULT
    OUTPUT_VARIABLE YARN_ADD_STDOUT
    ERROR_VARIABLE YARN_ADD_STDERR
)
if(NOT YARN_ADD_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install skymp-linter via yarn: ${YARN_ADD_STDERR}")
endif()

execute_process(
    COMMAND yarn global bin
    RESULT_VARIABLE YARN_BIN_RESULT
    OUTPUT_VARIABLE YARN_GLOBAL_BIN
    ERROR_VARIABLE YARN_BIN_STDERR
)
if(NOT YARN_BIN_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to detect yarn global bin: ${YARN_BIN_STDERR}")
endif()

string(STRIP "${YARN_GLOBAL_BIN}" YARN_GLOBAL_BIN)
set(SKYMP_LINTER_BIN "${YARN_GLOBAL_BIN}/skymp-linter")

if(NOT EXISTS ${SKYMP_LINTER_BIN})
    message(FATAL_ERROR "skymp-linter binary not found at ${SKYMP_LINTER_BIN}")
endif()

message(STATUS "Installing pre-commit hook via ${SKYMP_LINTER_BIN}")
execute_process(
    COMMAND ${SKYMP_LINTER_BIN} --install-hook
    WORKING_DIRECTORY ${REPO_ROOT}
    RESULT_VARIABLE HOOK_RESULT
    OUTPUT_VARIABLE HOOK_STDOUT
    ERROR_VARIABLE HOOK_STDERR
)
if(NOT HOOK_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install pre-commit hook: ${HOOK_STDERR}")
endif()

message(STATUS "Git hooks installed successfully via skymp-linter")
