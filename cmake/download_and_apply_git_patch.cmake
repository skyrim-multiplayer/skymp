function(download_and_apply_git_patch patch_url patch_file expected_sha512)
    # Download the patch file
    message(STATUS "Downloading patch file: ${patch_url}")
    file(DOWNLOAD ${patch_url} ${patch_file}
         TIMEOUT 60  # Adjust the timeout as necessary
         EXPECTED_HASH SHA512=${expected_sha512}
         STATUS download_status
         SHOW_PROGRESS)

    # Check if download was successful
    list(GET download_status 0 status_code)
    list(GET download_status 1 status_string)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Error downloading patch: ${status_string}")
    endif()

    # Check if the patch has already been applied
    execute_process(
        COMMAND git apply --check ${patch_file}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        RESULT_VARIABLE PATCH_APPLIED
        OUTPUT_QUIET
        ERROR_QUIET
    )

    # Apply the git patch if it has not been applied yet
    if(NOT PATCH_APPLIED EQUAL 0)
        message(STATUS "Applying git patch ${patch_file}")
        execute_process(
            COMMAND git apply ${patch_file}
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            RESULT_VARIABLE PATCH_RESULT
        )
        if(NOT PATCH_RESULT EQUAL 0)
            message(FATAL_ERROR "Failed to apply git patch: ${patch_file}")
        endif()
    else()
        message(STATUS "Patch ${patch_file} already applied")
    endif()
endfunction()
