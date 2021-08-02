cmake_minimum_required(VERSION 3.18.4)
project(client-deps)

if(WIN32)
  add_custom_target(client-deps ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/Data" "${CMAKE_BINARY_DIR}/dist/client/data"
  )

  add_dependencies(client-deps skyrim-platform)
endif()
