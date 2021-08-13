include(${CMAKE_CURRENT_LIST_DIR}/run_cppcoverage.cmake)

function(skymp_execute_process)
  set(options)
  set(oneValueArgs EXECUTABLE_PATH OUT_EXIT_CODE OUT_STDOUT OUT_STDERR CPPCOV_PATH CPPCOV CPPCOV_OUTPUT_DIRECTORY CPPCOV_TAG ATTACH)
  set(multiValueArgs CPPCOV_CMDLINE CPPCOV_SOURCES CPPCOV_MODULES)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  if(NOT EXISTS "${A_EXECUTABLE_PATH}")
    message(FATAL_ERROR "Executable with path '${A_EXECUTABLE_PATH}' doesn't exist")
  endif()

  foreach(v ${A_CPPCOV_SOURCES})
    string(REPLACE "/" "\\" v ${v})
    list(APPEND CPPCOV_CMDLINE --sources ${v})
  endforeach()
  foreach(v ${A_CPPCOV_MODULES})
    string(REPLACE "/" "\\" v ${v})
    list(APPEND CPPCOV_CMDLINE --modules ${v})
  endforeach()

  if(A_ATTACH)
    set(ATTACH_OPTION ATTACH)
  else()
    set(ATTACH_OPTION NO_ATTACH)
  endif()

  if(A_ATTACH)
    if(NOT A_CPPCOV)
      set(A_CPPCOV ON)
      set(A_CPPCOV_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cppcov_out_dummy)
      list(APPEND CPPCOV_CMDLINE --excluded_modules * --excluded_sources *)
      if(NOT A_CPPCOV_PATH)
        message(FATAL_ERROR "ATTACH mode requires CPPCOV_PATH argument regardless of CPPCOV argument")
      endif()
    endif()
  endif()

  if(A_CPPCOV)
    if(NOT A_CPPCOV_OUTPUT_DIRECTORY)
      message(FATAL_ERROR "CPPCOV_OUTPUT_DIRECTORY is not specified")
    endif()

    # Empty A_CPPCOV_PATH means that OpenCppCoverage is in PATH env
    if(NOT "${A_CPPCOV_PATH}" STREQUAL "" AND NOT EXISTS ${A_CPPCOV_PATH}/OpenCppCoverage.exe)
      message(FATAL_ERROR "'${A_CPPCOV_PATH}' is not a valid path to OpenCppCoverage.exe")
    endif()

    if("${A_CPPCOV_PATH}" STREQUAL "")
      set(CPPCOV_PATH_FULL "OpenCppCoverage.exe")
    else()
      set(CPPCOV_PATH_FULL "${A_CPPCOV_PATH}/OpenCppCoverage.exe")
      if(NOT EXISTS "${CPPCOV_PATH_FULL}")
        message(FATAL_ERROR "'${A_CPPCOV_PATH}' is not a valid path to OpenCppCoverage.exe")
      endif()
    endif()

    string(RANDOM LENGTH 16 str)
    set(str "coverage_${str}.tmp")
    set(tag_dir ${CMAKE_CURRENT_BINARY_DIR}/coverage_tag_${A_CPPCOV_TAG})

    set(input_cov "${tag_dir}/coverage.tmp")
    if(NOT EXISTS "${tag_dir}/coverage.tmp")
      set(input_cov)
    endif()
    run_cppcoverage(
      ${ATTACH_OPTION}
      INPUT_COVERAGE "${input_cov}"
      OPENCPPCOV_PATH ${CPPCOV_PATH_FULL}
      OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${str}
      EXECUTABLE_PATH ${A_EXECUTABLE_PATH}
      EXPORT_TYPE binary
      OUT_EXIT_CODE res
      OUT_ERR err
      WORKING_DIRECTORY ${A_CPPCOV_OUTPUT_DIRECTORY}
      CMDLINE ${CPPCOV_CMDLINE}
    )
    if(NOT "${A_OUT_STDERR}" STREQUAL "")
      set(${A_OUT_STDERR} ${err} PARENT_SCOPE)
    endif()

    run_cppcoverage(
      OPENCPPCOV_PATH ${CPPCOV_PATH_FULL}
      OUTPUT_DIRECTORY ${A_CPPCOV_OUTPUT_DIRECTORY}
      INPUT_COVERAGE ${CMAKE_CURRENT_BINARY_DIR}/${str}
      EXPORT_TYPE html
      WORKING_DIRECTORY ${A_CPPCOV_OUTPUT_DIRECTORY}
    )

    if(NOT "${A_CPPCOV_TAG}" STREQUAL "")
      file(COPY ${CMAKE_CURRENT_BINARY_DIR}/${str}
        DESTINATION ${tag_dir}
      )
      file(RENAME ${tag_dir}/${str}
        ${tag_dir}/coverage.tmp
      )
    endif()

  else()
    execute_process(COMMAND ${A_EXECUTABLE_PATH}
      RESULT_VARIABLE res
      OUTPUT_VARIABLE stdout
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  endif()

  set(${A_OUT_EXIT_CODE} ${res} PARENT_SCOPE)
  set(${A_OUT_STDOUT} ${stdout} PARENT_SCOPE)
endfunction()
