message(STATUS "Downloading gamemode.js")
file(DOWNLOAD ${GAMEMODE_JS_URL} ${GAMEMODE_JS_DEST}
     STATUS status
     LOG log
     TLS_VERIFY ON
     HTTPHEADER "Authorization: Bearer ${GITHUB_TOKEN}"
     )
list(GET status 0 status_code)
list(GET status 1 status_string)
if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "error: downloading gamemode.js failed: ${status_string}")
endif()
message(STATUS "Downloaded gamemode.js")
