message(STATUS "Downloading patches sources")

file(DOWNLOAD ${PATCHES_ZIP_URL} ${PATCHES_ZIP_DEST}
     STATUS status
     LOG log
     TLS_VERIFY ON
     HTTPHEADER "Authorization: token ${GITHUB_TOKEN}"
     )
list(GET status 0 status_code)
list(GET status 1 status_string)
if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "error: downloading patches sources failed: ${status_string}")
endif()

message(STATUS "Downloaded patches sources")

message(STATUS "Extracting patches sources")

# Execute the CMake command to extract the zip file
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xvf ${PATCHES_ZIP_DEST}
    RESULT_VARIABLE TAR_RESULT
    OUTPUT_QUIET # stop telling patches filenames in console
)

if(NOT TAR_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to extract patches zip file")
endif()

message(STATUS "Extracted patches sources")

message(STATUS "Installing patches")

file(COPY ${CMAKE_BINARY_DIR}/skymp5-patches/${PATCHES_REPO_NAME}-${PATCHES_BRANCH}/build/gamemode.js DESTINATION "${PATCHES_JS_DEST_DIR}")

message(STATUS "Installed patches")
