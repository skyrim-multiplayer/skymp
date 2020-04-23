function(cmake_generate)
  cmake_parse_arguments(A "" "NAME;SOURCE_DIR;BINARY_DIR;ARCHITECTURE;GENERATOR" "VARIABLES" ${ARGN})
  foreach(arg NAME SOURCE_DIR BINARY_DIR ARCHITECTURE GENERATOR)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  file(MAKE_DIRECTORY ${A_BINARY_DIR})

  set(cmdline ${CMAKE_COMMAND} -G ${A_GENERATOR} -A ${A_ARCHITECTURE})
  foreach(var ${A_VARIABLES})
    list(APPEND cmdline "-D${var}")
  endforeach()
  list(APPEND cmdline ${A_SOURCE_DIR})

  get_filename_component(dir_name ${A_SOURCE_DIR} NAME)

  message("\n\n")
  message(STATUS "Generating '${dir_name}'")
  message("\n\n")

  execute_process(COMMAND ${cmdline}
    WORKING_DIRECTORY ${A_BINARY_DIR}
    RESULT_VARIABLE res
  )
  if (NOT "${res}" STREQUAL "0")
    message(FATAL_ERROR "'${dir_name}' generation failed with '${res}'")
  endif()

  add_custom_target(${A_NAME} ALL
    COMMAND ${CMAKE_COMMAND} --build ${A_BINARY_DIR} --config $<CONFIG>
  )
endfunction()
