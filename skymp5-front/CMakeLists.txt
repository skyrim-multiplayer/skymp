project(skymp5-front)

set(FRONTEND_REPO_OWNER skyrim-multiplayer)
set(FRONTEND_REPO_NAME skymp5-front)
set(FRONTEND_REPO ${FRONTEND_REPO_OWNER}/${FRONTEND_REPO_NAME})
set(FRONTEND_BRANCH main)

if(BUILD_FRONT)
  if(NOT SKYMP5_FRONT_REPO_PAT)
    message(FATAL_ERROR "SKYMP5_FRONT_REPO_PAT is not set. Please set it to your GitHub personal access token.")
  endif()
  
  set(FRONTEND_ZIP_URL "https://github.com/${FRONTEND_REPO}/archive/refs/heads/${FRONTEND_BRANCH}.zip")
  set(FRONTEND_ZIP_DEST "${CMAKE_BINARY_DIR}/frontend.zip")
  set(FRONTEND_ZIP_EXTRACT_DIR "${CMAKE_BINARY_DIR}/frontend-zip")
  set(FRONTEND_JS_DEST_DIR "${CMAKE_BINARY_DIR}/dist/client/Data/Platform/UI")

  set(DEPLOY_BRANCH "$ENV{DEPLOY_BRANCH}")
  set(AUTO_MERGE_REPO "Pospelove/auto-merge-action")
  set(AUTO_MERGE_BRANCH "main")
  set(AUTO_MERGE_REPO_URL "https://github.com/${AUTO_MERGE_REPO}.git")
  set(FRONTEND_REPO_URL "https://${SKYMP5_FRONT_REPO_PAT}:x-oauth-basic@github.com/${FRONTEND_REPO_OWNER}/${FRONTEND_REPO_NAME}.git")

  if("$ENV{DEPLOY_BRANCH}" STREQUAL "")
    set(ENV_INPUT_REPOSITORIES "[{\"owner\": \"skyrim-multiplayer\", \"repo\": \"skymp5-frontend\", \"labels\": [], \"token\": \"${SKYMP5_FRONT_REPO_PAT}\"}]")
  else()
    set(ENV_INPUT_REPOSITORIES "[{\"owner\": \"skyrim-multiplayer\", \"repo\": \"skymp5-frontend\", \"labels\": [\"merge-to:${DEPLOY_BRANCH}\"], \"token\": \"${SKYMP5_FRONT_REPO_PAT}\"}]")
  endif()

  # Pass variables to the download script with PRS.
  set(SCRIPT_PATH "${CMAKE_CURRENT_LIST_DIR}/download-and-build-with-prs.cmake")
  configure_file(${SCRIPT_PATH} ${CMAKE_BINARY_DIR}/download-and-build-with-prs.cmake ESCAPE_QUOTES)

  set(source download-and-build-with-prs.cmake)

  add_custom_target(skymp5-front ALL
    SOURCES ${source}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/client
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}/${FRONTEND_REPO_NAME}-${FRONTEND_BRANCH}
    COMMAND ${CMAKE_COMMAND} -E env DOWNLOAD_NO_PROGRESS=1
            ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/${source}
    COMMENT "Downloading & building frontend repo..."
  )
else()
  add_custom_target(skymp5-front ALL
    SOURCES ${source}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-front is disabled. To enable it, set BUILD_FRONT to ON."
  )
endif()
