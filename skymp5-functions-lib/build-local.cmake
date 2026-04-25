if(NOT DEFINED GAMEMODE_SOURCE_DIR OR "${GAMEMODE_SOURCE_DIR}" STREQUAL "")
  message(FATAL_ERROR "GAMEMODE_SOURCE_DIR is not set")
endif()

if(NOT DEFINED GAMEMODE_JS_DEST_DIR OR "${GAMEMODE_JS_DEST_DIR}" STREQUAL "")
  message(FATAL_ERROR "GAMEMODE_JS_DEST_DIR is not set")
endif()

file(TO_CMAKE_PATH "${GAMEMODE_SOURCE_DIR}" GAMEMODE_SOURCE_DIR)
file(TO_CMAKE_PATH "${GAMEMODE_JS_DEST_DIR}" GAMEMODE_JS_DEST_DIR)

set(GAMEMODE_OUTPUT_PATH "${GAMEMODE_JS_DEST_DIR}/gamemode.js")
set(LEGACY_GAMEMODE_FOLDER "${GAMEMODE_JS_DEST_DIR}/skymp5-gamemode")

if(NOT EXISTS "${GAMEMODE_SOURCE_DIR}")
  message(FATAL_ERROR "Local gamemode source directory does not exist: ${GAMEMODE_SOURCE_DIR}")
endif()

if(NOT EXISTS "${GAMEMODE_SOURCE_DIR}/package.json")
  message(FATAL_ERROR "Local gamemode package.json does not exist: ${GAMEMODE_SOURCE_DIR}/package.json")
endif()

if(WIN32)
  set(NPM_EXECUTABLE npm.cmd)
else()
  set(NPM_EXECUTABLE npm)
endif()

message(STATUS "Building local gamemode -> ${GAMEMODE_OUTPUT_PATH}")

file(MAKE_DIRECTORY "${GAMEMODE_JS_DEST_DIR}")
file(REMOVE_RECURSE "${LEGACY_GAMEMODE_FOLDER}")

execute_process(
  COMMAND ${CMAKE_COMMAND} -E env "GAMEMODE_OUTPUT_DIR=${GAMEMODE_JS_DEST_DIR}" ${NPM_EXECUTABLE} run build
  WORKING_DIRECTORY "${GAMEMODE_SOURCE_DIR}"
  RESULT_VARIABLE GAMEMODE_BUILD_RESULT
)

if(NOT GAMEMODE_BUILD_RESULT EQUAL 0)
  message(FATAL_ERROR "Local gamemode build failed with exit code ${GAMEMODE_BUILD_RESULT}")
endif()

if(NOT EXISTS "${GAMEMODE_OUTPUT_PATH}")
  message(FATAL_ERROR "Local gamemode build did not produce ${GAMEMODE_OUTPUT_PATH}")
endif()

message(STATUS "Installed local gamemode.js")
