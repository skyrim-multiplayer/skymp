function(yarn_execute_command)
  cmake_parse_arguments(A "" "WORKING_DIRECTORY;OUTPUT_VARIABLE;RESULT_VARIABLE" "COMMAND" ${ARGN})
  foreach(arg WORKING_DIRECTORY COMMAND)
    if("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  # https://github.com/skyrim-multiplayer/skymp/issues/55
  if(NOT IS_ABSOLUTE "${A_WORKING_DIRECTORY}")
    message(FATAL_ERROR "Expected WORKING_DIRECTORY to be an absolute path, but got: ${A_WORKING_DIRECTORY}")
  endif()

  if(WIN32)
    set(temp_bat "${CMAKE_CURRENT_BINARY_DIR}/temp.bat")
    set(yarn_cmd ${temp_bat})
    set(yarn_arg "")
    set(str "yarn")
    foreach(arg ${A_COMMAND})
      string(APPEND str " ${arg}")
    endforeach()
    file(WRITE ${temp_bat} ${str})
  else()
    set(yarn_cmd "yarn")
    set(yarn_arg ${A_COMMAND})
  endif()

  execute_process(COMMAND ${yarn_cmd} ${yarn_arg}
    WORKING_DIRECTORY ${A_WORKING_DIRECTORY}
    RESULT_VARIABLE yarn_result
    OUTPUT_VARIABLE yarn_output
    ERROR_VARIABLE yarn_error
  )

  if("${A_RESULT_VARIABLE}" STREQUAL "")
    if(NOT "${yarn_result}" STREQUAL "0")
      message(FATAL_ERROR "yarn ${A_COMMAND} exited with ${yarn_result}:\n${yarn_output}\n${yarn_error}")
    endif()
  else()
    set("${A_RESULT_VARIABLE}" "${yarn_result}" PARENT_SCOPE)
  endif()

  set("${A_OUTPUT_VARIABLE}" "${yarn_output}" PARENT_SCOPE)
endfunction()

function(yarn_set_script)
  cmake_parse_arguments(A "" "WORKING_DIRECTORY;NAME;RESULT_VARIABLE;OUTPUT_VARIABLE" "SCRIPT" ${ARGN})
  foreach(arg WORKING_DIRECTORY NAME SCRIPT RESULT_VARIABLE OUTPUT_VARIABLE)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  foreach(arg ${A_SCRIPT})
    string(APPEND script "${arg} ")
  endforeach()

  set(js_src "
    var fs = require('fs');
    var packageJson = JSON.parse(fs.readFileSync('package.json'));
    packageJson.scripts['${A_NAME}'] = '${script}';
    fs.writeFileSync('package.json', JSON.stringify(packageJson, null, 2));
  ")

  execute_process(COMMAND node -p "${js_src}"
    WORKING_DIRECTORY ${A_WORKING_DIRECTORY}
    RESULT_VARIABLE res
    OUTPUT_VARIABLE out
    # TODO: ERROR_VARIABLE
  )
  set("${A_RESULT_VARIABLE}" "${res}" PARENT_SCOPE)
  set("${A_OUTPUT_VARIABLE}" "${out}" PARENT_SCOPE)
endfunction()
