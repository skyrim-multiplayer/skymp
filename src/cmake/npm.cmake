function(npm_execute_command)
  cmake_parse_arguments(A "" "WORKING_DIRECTORY;OUTPUT_VARIABLE;RESULT_VARIABLE" "COMMAND" ${ARGN})
  foreach(arg WORKING_DIRECTORY COMMAND)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  if (WIN32)
    set(temp_bat "${CMAKE_CURRENT_BINARY_DIR}/temp.bat")
    set(npm_cmd ${temp_bat})
    set(npm_arg "")
    set(str "npm")
    foreach(arg ${A_COMMAND})
      string(APPEND str " ${arg}")
    endforeach()
    file(WRITE ${temp_bat} ${str})
  else()
    set(npm_cmd "npm")
    set(npm_arg ${A_COMMAND})
  endif()

  execute_process(COMMAND ${npm_cmd} ${npm_arg}
    WORKING_DIRECTORY ${A_WORKING_DIRECTORY}
    RESULT_VARIABLE npm_result
    OUTPUT_VARIABLE npm_output
    # TODO: ERROR_VARIABLE
  )

  if ("${A_RESULT_VARIABLE}" STREQUAL "")
    if (NOT "${npm_result}" STREQUAL "0")
      message(FATAL_ERROR "npm ${A_COMMAND} exited with ${npm_result}:\n${npm_output}")
    endif()
  else()
    set("${A_RESULT_VARIABLE}" "${npm_result}" PARENT_SCOPE)
  endif()

  set("${A_OUTPUT_VARIABLE}" "${npm_output}" PARENT_SCOPE)
endfunction()

function(npm_set_script)
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
