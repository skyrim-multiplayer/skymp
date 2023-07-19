project(skymp5-functions-lib)

set(GAMEMODE_REPO Pospelove/skymp5-gamemode)
set(GAMEMODE_REVISION 5138f1f8a4010e4ecb75b6c2975cfc114d40976e)

if(BUILD_GAMEMODE)
  if(NOT GITHUB_TOKEN)
    message(FATAL_ERROR "GITHUB_TOKEN is not set. Please set it to your GitHub personal access token.")
  endif()
  # The URL to the raw gamemode.js file.
  set(GAMEMODE_JS_URL "https://raw.githubusercontent.com/${GAMEMODE_REPO}/${GAMEMODE_REVISION}/build/gamemode.js")

  # The destination where the file will be saved.
  set(GAMEMODE_JS_DEST "${CMAKE_BINARY_DIR}/dist/server/gamemode.js")

  # Pass variables to the download script.
  set(SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/download.cmake")
  configure_file(${SCRIPT_PATH} ${CMAKE_BINARY_DIR}/download.cmake)

  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E env DOWNLOAD_NO_PROGRESS=1
            ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/download.cmake
    COMMENT "Downloading gamemode.js file..."
  )
else()
  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-functions-lib is disabled. To enable it, set BUILD_GAMEMODE to ON."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E true > ${CMAKE_BINARY_DIR}/dist/server/gamemode.js
  )
endif()
