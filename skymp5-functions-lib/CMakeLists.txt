project(skymp5-functions-lib)

set(GAMEMODE_REPO_OWNER Pospelove)
set(GAMEMODE_REPO_NAME skymp5-gamemode)
set(GAMEMODE_REPO ${GAMEMODE_REPO_OWNER}/${GAMEMODE_REPO_NAME})
set(GAMEMODE_BRANCH master)

if(BUILD_GAMEMODE)
  if(NOT GITHUB_TOKEN)
    message(FATAL_ERROR "GITHUB_TOKEN is not set. Please set it to your GitHub personal access token.")
  endif()
  
  set(GAMEMODE_ZIP_URL "https://github.com/${GAMEMODE_REPO}/archive/refs/heads/${GAMEMODE_BRANCH}.zip")
  set(GAMEMODE_ZIP_DEST "${CMAKE_BINARY_DIR}/gamemode.zip")
  set(GAMEMODE_ZIP_EXTRACT_DIR "${CMAKE_BINARY_DIR}/gamemode-zip")
  set(GAMEMODE_JS_DEST_DIR "${CMAKE_BINARY_DIR}/dist/server")

  # Pass variables to the download script.
  set(SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/download-and-build.cmake")
  configure_file(${SCRIPT_PATH} ${CMAKE_BINARY_DIR}/download-and-build.cmake)

  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/${GAMEMODE_REPO_NAME}-${GAMEMODE_BRANCH}
    COMMAND ${CMAKE_COMMAND} -E env DOWNLOAD_NO_PROGRESS=1
            ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/download-and-build.cmake
    COMMENT "Downloading & building gamemode repo..."
  )
else()
  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-functions-lib is disabled. To enable it, set BUILD_GAMEMODE to ON."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E true > ${CMAKE_BINARY_DIR}/dist/server/gamemode.js
  )
endif()
