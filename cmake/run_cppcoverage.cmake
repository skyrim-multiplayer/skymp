function(run_cppcoverage)
  set(options ATTACH)
  set(oneValueArgs OUTPUT_DIRECTORY EXECUTABLE_PATH OPENCPPCOV_PATH OUT_EXIT_CODE OUT_ERR EXPORT_TYPE INPUT_COVERAGE WORKING_DIRECTORY)
  set(multiValueArgs CMDLINE)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  if(NOT WIN32)
    return()
  endif()

  file(REMOVE ${CMAKE_BINARY_DIR}/LastCoverageResults.log)

  if(EXISTS ${A_OUTPUT_DIRECTORY}/index.html)
    file(REMOVE_RECURSE ${A_OUTPUT_DIRECTORY})
  endif()

  set(ENV{OpenCppCoverage_Attach} "")
  if(A_ATTACH STREQUAL "TRUE")
    set(ENV{OpenCppCoverage_Attach} "1")
  endif()

  set(var ${A_OPENCPPCOV_PATH})
  message(STATUS "Starting ${var}")

  set(process_args ${A_CMDLINE})
  list(APPEND process_args 
    --excluded_sources *\\crt\\*
    --excluded_sources *\\unifiedcrt\\*
    --excluded_sources *\\program*microsoft*
    --excluded_sources *\\windows*
    --excluded_sources *\\vcpkg\\*
    --excluded_sources *\\vcpkg_installed\\*
    --excluded_sources *\\crts\\*
    --excluded_sources *\\vctools\\*
    --excluded_sources *\\ucrt\\*
    --excluded_sources *\\vcruntime\\*
    --excluded_sources *\\unit\\*
    --excluded_sources *\\third_party\\*
    --export_type "${A_EXPORT_TYPE}:${A_OUTPUT_DIRECTORY}"
  )

  if(NOT ${A_INPUT_COVERAGE} STREQUAL "")
    list(APPEND process_args --input_coverage ${A_INPUT_COVERAGE})
  endif()

  list(APPEND process_args --verbose)
  if(NOT "${A_EXECUTABLE_PATH}" STREQUAL "")
    list(APPEND process_args -- ${A_EXECUTABLE_PATH})
  endif()

  execute_process(COMMAND ${var} ${process_args}
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
  )
  if(NOT "${res}" STREQUAL "0")
    message(FATAL_ERROR "OpenCppCoverage exited with code ${res}")
  endif()
  message(STATUS "OpenCppCoverage exited with code ${res}")

  set(${A_OUT_ERR} ${err} PARENT_SCOPE)
  set(${A_OUT_EXIT_CODE} ${res} PARENT_SCOPE)

  file(MAKE_DIRECTORY "${A_WORKING_DIRECTORY}")
  if(EXISTS "${A_WORKING_DIRECTORY}")
    file(WRITE ${A_WORKING_DIRECTORY}/OpenCppCoverage-stderr.log ${err})
    file(WRITE ${A_WORKING_DIRECTORY}/OpenCppCoverage-stdout.log ${out})
    file(COPY ${CMAKE_BINARY_DIR}/LastCoverageResults.log
      DESTINATION ${A_WORKING_DIRECTORY}
    )
  endif()
endfunction()
