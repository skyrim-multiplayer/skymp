include(${CMAKE_CURRENT_LIST_DIR}/yarn.cmake)

yarn_execute_command(
  WORKING_DIRECTORY ${SKYMP5_FUNCTIONS_LIB_DIR}
  COMMAND test
)
