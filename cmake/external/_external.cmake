include(ExternalProject)

set(EXTERNAL_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(EXTERNAL_BINARY_DIR ${CMAKE_BINARY_DIR}/external_build)

file(MAKE_DIRECTORY ${EXTERNAL_BINARY_DIR})

message(STATUS "[external] Generating")
execute_process(
  COMMAND ${CMAKE_COMMAND} ${EXTERNAL_SOURCE_DIR}
  WORKING_DIRECTORY ${EXTERNAL_BINARY_DIR}
  RESULT_VARIABLE res
)
message(STATUS "[external] Generation finished with code ${res}")
if(NOT res EQUAL "0")
    message(FATAL_ERROR "[external] Bad exit status: ${res}")
endif()

message(STATUS "[external] Building")
execute_process(
  COMMAND ${CMAKE_COMMAND} --build .
  WORKING_DIRECTORY ${EXTERNAL_BINARY_DIR}
  RESULT_VARIABLE res
)
message(STATUS "[external] Building finished with code ${res}")
if(NOT res EQUAL "0")
    message(FATAL_ERROR "[external] Bad exit status: ${res}")
endif()
