cmake_minimum_required(VERSION 3.19.1)
project(client-deps)

add_custom_target(client-deps ALL
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  SOURCES ${sources}
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/Data" "${CMAKE_BINARY_DIR}/dist/client/data"
)

add_dependencies(client-deps skyrim-platform)