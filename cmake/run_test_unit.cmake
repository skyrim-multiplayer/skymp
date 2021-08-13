include(${CMAKE_CURRENT_LIST_DIR}/skymp_execute_process.cmake)

string(RANDOM LENGTH 5 tag)

skymp_execute_process(
  EXECUTABLE_PATH ${EXE_PATH}
  CPPCOV ${CPPCOV}
  CPPCOV_TAG ${tag}
  CPPCOV_PATH ${CPPCOV_PATH}
  CPPCOV_MODULES *
  CPPCOV_SOURCES *
  CPPCOV_OUTPUT_DIRECTORY ${COVERAGE_HTML_OUT_DIR}
  OUT_EXIT_CODE EXIT_CODE
  OUT_STDOUT STDOUT
)

if(NOT "${EXIT_CODE}" STREQUAL "0")
  message(FATAL_ERROR "Bad exit status ${EXIT_CODE}")
endif()

execute_process(COMMAND ${EXE_PATH}
  RESULT_VARIABLE res
  OUTPUT_VARIABLE out
)

message("${out}")
