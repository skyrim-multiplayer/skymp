project(skymp5-functions-lib)

include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

yarn_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

add_custom_target(skymp5-functions-lib ALL
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  SOURCES ${sources}
  COMMAND yarn build
)
add_dependencies(skymp5-functions-lib skyrim-platform)
