include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

message(STATUS "Downloading gamemode sources")

file(DOWNLOAD ${GAMEMODE_ZIP_URL} ${GAMEMODE_ZIP_DEST}
     STATUS status
     LOG log
     TLS_VERIFY ON
     HTTPHEADER "Authorization: token ${GITHUB_TOKEN}"
     )
list(GET status 0 status_code)
list(GET status 1 status_string)
if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "error: downloading gamemode sources failed: ${status_string}")
endif()

message(STATUS "Downloaded gamemode sources")

message(STATUS "Extracting gamemode sources")

# Execute the CMake command to extract the zip file
execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xvf ${GAMEMODE_ZIP_DEST}
    RESULT_VARIABLE TAR_RESULT
    OUTPUT_QUIET # stop telling gamemode filenames in console
)

if(NOT TAR_RESULT EQUAL "0")
    message(FATAL_ERROR "Failed to extract gamemode zip file")
endif()

message(STATUS "Extracted gamemode sources")

message(STATUS "Installing yarn dependencies for gamemode")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/skymp5-functions-lib/${GAMEMODE_REPO_NAME}-${GAMEMODE_BRANCH}
    COMMAND install
)

message(STATUS "Installed yarn dependencies for gamemode")

message(STATUS "Building gamemode.js")

yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/skymp5-functions-lib/${GAMEMODE_REPO_NAME}-${GAMEMODE_BRANCH}
    COMMAND build
)

message(STATUS "Built gamemode.js")

message(STATUS "Installing gamemode.js")

file(COPY ${CMAKE_BINARY_DIR}/skymp5-functions-lib/${GAMEMODE_REPO_NAME}-${GAMEMODE_BRANCH}/build/gamemode.js DESTINATION "${GAMEMODE_JS_DEST_DIR}")

message(STATUS "Installed gamemode.js")
