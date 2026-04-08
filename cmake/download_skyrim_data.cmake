function(download_skyrim_data DEST_DIR)
    if(NOT EXISTS "${DEST_DIR}")
        file(MAKE_DIRECTORY "${DEST_DIR}")
    endif()

    set(URL_BASE "https://gitlab.com/pospelov/se-data/-/raw/main")
    set(FILES
        "Skyrim.esm"
        "Update.esm"
        "Dawnguard.esm"
        "HearthFires.esm"
        "Dragonborn.esm"
    )

    foreach(FILE_NAME ${FILES})
        set(FILE_PATH "${DEST_DIR}/${FILE_NAME}")
        if(NOT EXISTS "${FILE_PATH}")
            message(STATUS "Downloading ${FILE_NAME} to ${DEST_DIR}...")
            file(DOWNLOAD "${URL_BASE}/${FILE_NAME}" "${FILE_PATH}"
                STATUS status
                LOG log
            )
            list(GET status 0 status_code)
            list(GET status 1 status_string)

            if(NOT status_code EQUAL 0)
                message(FATAL_ERROR "error: downloading '${FILE_NAME}' failed
  status_code: ${status_code}
  status_string: ${status_string}
  log: ${log}
")
            endif()
        else()
             message(STATUS "${FILE_NAME} already exists in ${DEST_DIR}")
        endif()
    endforeach()
endfunction()
