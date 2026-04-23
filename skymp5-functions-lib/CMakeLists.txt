project(skymp5-functions-lib)

set(GAMEMODE_REPO_OWNER skyrim-multiplayer)
set(GAMEMODE_REPO_NAME skymp5-gamemode)
set(GAMEMODE_REPO ${GAMEMODE_REPO_OWNER}/${GAMEMODE_REPO_NAME})
set(GAMEMODE_BRANCH master)
set(GAMEMODE_SOURCE_DIR "${CMAKE_SOURCE_DIR}/skymp5-gamemode")
set(GAMEMODE_JS_DEST_DIR "${CMAKE_BINARY_DIR}/dist/server")
set(GAMEMODE_LOCAL_BUILD_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/build-local.cmake")

if(BUILD_GAMEMODE)
  if(BUILD_GAMEMODE_FROM_REMOTE)
    if(NOT GITHUB_TOKEN)
      message(FATAL_ERROR "GITHUB_TOKEN is not set. Please set it to your GitHub personal access token.")
    endif()

    set(GAMEMODE_ZIP_URL "https://github.com/${GAMEMODE_REPO}/archive/refs/heads/${GAMEMODE_BRANCH}.zip")
    set(GAMEMODE_ZIP_DEST "${CMAKE_BINARY_DIR}/gamemode.zip")
    set(GAMEMODE_ZIP_EXTRACT_DIR "${CMAKE_BINARY_DIR}/gamemode-zip")

    set(DEPLOY_BRANCH "$ENV{DEPLOY_BRANCH}")
    set(AUTO_MERGE_REPO "Pospelove/auto-merge-action")
    set(AUTO_MERGE_BRANCH "main")
    set(AUTO_MERGE_REPO_URL "https://github.com/${AUTO_MERGE_REPO}.git")
    set(GAMEMODE_REPO_URL "https://${GITHUB_TOKEN}:x-oauth-basic@github.com/${GAMEMODE_REPO_OWNER}/${GAMEMODE_REPO_NAME}.git")

    if("$ENV{DEPLOY_BRANCH}" STREQUAL "")
      set(ENV_INPUT_REPOSITORIES "[{\"owner\": \"skyrim-multiplayer\", \"repo\": \"skymp5-gamemode\", \"labels\": [], \"token\": \"${GITHUB_TOKEN}\"}]")
    else()
      set(ENV_INPUT_REPOSITORIES "[{\"owner\": \"skyrim-multiplayer\", \"repo\": \"skymp5-gamemode\", \"labels\": [\"merge-to:${DEPLOY_BRANCH}\"], \"token\": \"${GITHUB_TOKEN}\"}]")
    endif()

    # Pass variables to the download script with PRS.
    set(SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/download-and-build-with-prs.cmake")
    configure_file(${SCRIPT_PATH} ${CMAKE_BINARY_DIR}/download-and-build-with-prs.cmake ESCAPE_QUOTES)

    set(source download-and-build-with-prs.cmake)

    add_custom_target(skymp5-functions-lib ALL
      SOURCES ${source}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GAMEMODE_JS_DEST_DIR}
      COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/${GAMEMODE_REPO_NAME}-${GAMEMODE_BRANCH}
      COMMAND ${CMAKE_COMMAND} -E env DOWNLOAD_NO_PROGRESS=1
              ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/${source}
      COMMENT "Downloading & building gamemode repo..."
      VERBATIM
    )
  else()
    add_custom_target(skymp5-functions-lib ALL
      SOURCES ${GAMEMODE_LOCAL_BUILD_SCRIPT}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${GAMEMODE_JS_DEST_DIR}
      COMMAND ${CMAKE_COMMAND}
              "-DGAMEMODE_SOURCE_DIR=${GAMEMODE_SOURCE_DIR}"
              "-DGAMEMODE_JS_DEST_DIR=${GAMEMODE_JS_DEST_DIR}"
              -P ${GAMEMODE_LOCAL_BUILD_SCRIPT}
      COMMENT "Building gamemode from local sources..."
      VERBATIM
    )
  endif()
else()
  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${GAMEMODE_LOCAL_BUILD_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-functions-lib is disabled. To enable it, set BUILD_GAMEMODE to ON."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${GAMEMODE_JS_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E touch "${GAMEMODE_JS_DEST_DIR}/gamemode.js"
    VERBATIM
  )
endif()
