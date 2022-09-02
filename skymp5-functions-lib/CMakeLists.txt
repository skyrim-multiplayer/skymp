project(skymp5-functions-lib)

include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

yarn_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

add_custom_target(skymp5-functions-lib ALL
  SOURCES ${sources}
  COMMAND yarn --cwd "\"${CMAKE_CURRENT_LIST_DIR}\"" build
)
add_dependencies(skymp5-functions-lib skyrim-platform)

add_test(
  NAME test_functions_lib
  COMMAND ${CMAKE_COMMAND}
    -DSKYMP5_FUNCTIONS_LIB_DIR=${CMAKE_CURRENT_LIST_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/run_test_functions_lib.cmake
)

add_dependencies(skymp5-functions-lib skyrim-platform)