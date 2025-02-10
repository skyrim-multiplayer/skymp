# A utility script for Git Hooks installation
# Usage:
# cmake -P misc/install_git_hooks.cmake

set(REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
get_filename_component(REPO_ROOT ${REPO_ROOT} REALPATH)

file(MAKE_DIRECTORY ${REPO_ROOT}/.git/hooks)

# Copy pre-commit hook
file(COPY ${REPO_ROOT}/misc/git-hooks/pre-commit DESTINATION ${REPO_ROOT}/.git/hooks)


# Make pre-commit hook executable if running on Unix
if(UNIX)
    execute_process(COMMAND chmod +x ${REPO_ROOT}/.git/hooks/pre-commit)
endif()

# Install dependencies

yarn_execute_command(
    WORKING_DIRECTORY ${REPO_ROOT}/misc/git-hooks
    COMMAND add simple-git
)
