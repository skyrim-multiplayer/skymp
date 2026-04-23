if(NOT DEFINED FRONTEND_SOURCE_DIR OR "${FRONTEND_SOURCE_DIR}" STREQUAL "")
  message(FATAL_ERROR "FRONTEND_SOURCE_DIR is not set")
endif()

if(NOT DEFINED FRONTEND_JS_DEST_DIR OR "${FRONTEND_JS_DEST_DIR}" STREQUAL "")
  message(FATAL_ERROR "FRONTEND_JS_DEST_DIR is not set")
endif()

if(NOT DEFINED YARN_HELPERS_PATH OR "${YARN_HELPERS_PATH}" STREQUAL "")
  message(FATAL_ERROR "YARN_HELPERS_PATH is not set")
endif()

include("${YARN_HELPERS_PATH}")

file(TO_CMAKE_PATH "${FRONTEND_SOURCE_DIR}" FRONTEND_SOURCE_DIR)
file(TO_CMAKE_PATH "${FRONTEND_JS_DEST_DIR}" FRONTEND_JS_DEST_DIR)

set(FRONTEND_CONFIG_PATH "${FRONTEND_SOURCE_DIR}/config.js")

message(STATUS "Installing yarn dependencies for skymp5-front from local sources")
yarn_execute_command(
  WORKING_DIRECTORY "${FRONTEND_SOURCE_DIR}"
  COMMAND install
)

message(STATUS "Writing config.js for skymp5-front")
file(WRITE "${FRONTEND_CONFIG_PATH}" "")
file(APPEND "${FRONTEND_CONFIG_PATH}" "module.exports = {\n")
file(APPEND "${FRONTEND_CONFIG_PATH}" "  outputPath:\n")
file(APPEND "${FRONTEND_CONFIG_PATH}" "    '${FRONTEND_JS_DEST_DIR}',\n")
file(APPEND "${FRONTEND_CONFIG_PATH}" "};\n")

message(STATUS "Building skymp5-front from local sources")
yarn_execute_command(
  WORKING_DIRECTORY "${FRONTEND_SOURCE_DIR}"
  COMMAND build
)

message(STATUS "Built skymp5-front from local sources")
