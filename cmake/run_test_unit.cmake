include(${CMAKE_CURRENT_LIST_DIR}/third_party/cmake_scripts_collection.cmake)

# Run without OpenCppCoverage first:

if(CPPCOV)
  execute_process(COMMAND ${EXE_PATH}
    RESULT_VARIABLE res
    OUTPUT_VARIABLE out
  )
  if(NOT "${res}" STREQUAL "0")
    message("${out}")
    message(FATAL_ERROR "Exited with ${res}")
  endif()
endif()

# Run under OpenCppCoverage:

determine_cppcov_tag(OUTPUT_VARIABLE tag)

skymp_execute_process(
  EXECUTABLE_PATH ${EXE_PATH}
  CPPCOV ${CPPCOV}
  CPPCOV_TAG ${tag}
  CPPCOV_PATH ${OPENCPPCOV_DIR}
  CPPCOV_MODULES *
  CPPCOV_SOURCES *\\mp_common\\*
  CPPCOV_OUTPUT_DIRECTORY ${COVERAGE_HTML_OUT_DIR}
  OUT_EXIT_CODE EXIT_CODE
  OUT_STDOUT STDOUT
)

if(NOT "${EXIT_CODE}" STREQUAL "0")
  message("${STDOUT}")
  message(FATAL_ERROR "Exited with ${EXIT_CODE}")
endif()
