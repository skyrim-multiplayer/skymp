# run_integration_test.cmake
#
# Runs a single integration JS test inside skymp5-server.
#
# Required variables (pass via -D):
#   TEST_SCRIPT  — absolute path to the test .js file
#   SERVER_DIR   — absolute path to the server directory (build/dist/server)
#
# Optional variables:
#   TEST_DB_DIR  — absolute path to a satellite folder with initial changeForms DB

# 1. Clean changeForms from previous runs
set(CHANGE_FORMS_DIR "${SERVER_DIR}/world/changeForms")
if(EXISTS "${CHANGE_FORMS_DIR}")
  file(REMOVE_RECURSE "${CHANGE_FORMS_DIR}")
endif()

# 2. If a satellite DB folder is provided, copy it into changeForms
if(TEST_DB_DIR AND EXISTS "${TEST_DB_DIR}")
  file(COPY "${TEST_DB_DIR}/" DESTINATION "${CHANGE_FORMS_DIR}")
endif()

# 3. Copy the test script as gamemode.js
file(COPY "${TEST_SCRIPT}" DESTINATION "${SERVER_DIR}")
get_filename_component(TEST_FILENAME "${TEST_SCRIPT}" NAME)
file(RENAME "${SERVER_DIR}/${TEST_FILENAME}" "${SERVER_DIR}/gamemode.js")

# 4. Run the server
execute_process(
  COMMAND node dist_back/skymp5-server.js
  WORKING_DIRECTORY "${SERVER_DIR}"
  TIMEOUT 60
  RESULT_VARIABLE res
)

# 5. Check result
if(NOT "${res}" STREQUAL "0")
  message(FATAL_ERROR "Integration test failed with exit code: ${res}")
endif()
