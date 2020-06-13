
# It's a collection of CMake scripts combined into a single file automatically
# See build.cmake

#
# apply_default_settings.cmake:
#

function(apply_default_settings)
  cmake_parse_arguments(A "" "" "TARGETS" ${ARGN})
  foreach(arg TARGETS)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  foreach(target ${A_TARGETS})
    set_target_properties(${target} PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
      CXX_EXTENSIONS OFF
      CXX_STANDARD 17
    )
    if (MSVC)
      set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " /DEBUG /OPT:REF ")
      target_compile_options(${target} PUBLIC "/MP")
      target_compile_options(${target} PUBLIC "$<$<CONFIG:Release>:/Zi>")
      #target_compile_options(${target} PUBLIC "/permissive-")
    endif()
  endforeach()

endfunction()


#
# cmake_generate.cmake:
#

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


#
# determine_cppcov_tag.cmake:
#

function(determine_cppcov_tag)
  set(options)
  set(oneValueArgs OUTPUT_VARIABLE)
  set(multiValueArgs)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  if ("${A_OUTPUT_VARIABLE}" STREQUAL "")
    message(FATAL_ERROR "Missing OUTPUT_VARIABLE")
  endif()

  file(READ ${CMAKE_CURRENT_BINARY_DIR}/cppcov_tag.tmp tag)
  if("${tag}" STREQUAL "")
    message(FATAL_ERROR "Unable to determine CPPCOV_TAG")
  endif()
  set("${A_OUTPUT_VARIABLE}" "${tag}" PARENT_SCOPE)
endfunction()


#
# integrate_vcpkg.cmake:
#

function(integrate_vcpkg)
  cmake_parse_arguments(A "" "VCPKG_PATH" "TARGETS;ADDITIONAL_INCLUDE_DIRS" ${ARGN})
  foreach(arg VCPKG_PATH TARGETS)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  if (NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
    set(platform ${CMAKE_GENERATOR_PLATFORM})
  elseif(NOT "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "")
    set(platform ${CMAKE_VS_PLATFORM_NAME})
  else()
    message(FATAL_ERROR "Enable to detect current platform")
  endif()

  if (WIN32)
    set(os windows)
  elseif(UNIX)
    set(os linux)
  else()
    message(FATAL_ERROR "Only Windows and Linux are supported")
  endif()

  set(triplet_prefix_Win32 "x86")
  set(triplet_prefix_x64 "x64")
  set(triplet "${triplet_prefix_${platform}}-${os}-static")

  message(STATUS "[integrate_vcpkg] platform is ${platform}, triplet is ${triplet}")

  foreach(target ${A_TARGETS})
    target_include_directories(${target} PUBLIC
      "${A_VCPKG_PATH}/installed/${triplet}/include"
    )
    foreach(dir ${A_ADDITIONAL_INCLUDE_DIRS})
      target_include_directories(${target} PUBLIC
        "${A_VCPKG_PATH}/installed/${triplet}/include/${dir}"
      )
    endforeach()
    file(GLOB_RECURSE release_libs "${A_VCPKG_PATH}/installed/${triplet}/lib/*")
    file(GLOB_RECURSE debug_libs "${A_VCPKG_PATH}/installed/${triplet}/debug/lib/*")
    target_link_libraries(${target} PUBLIC debug ${debug_libs} optimized ${release_libs})
  endforeach()
endfunction()


#
# npm.cmake:
#

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


#
# run_cppcoverage.cmake:
#

function(run_cppcoverage)
  set(options ATTACH)
  set(oneValueArgs OUTPUT_DIRECTORY EXECUTABLE_PATH OPENCPPCOV_PATH OUT_EXIT_CODE OUT_ERR EXPORT_TYPE INPUT_COVERAGE WORKING_DIRECTORY)
  set(multiValueArgs CMDLINE)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  if (NOT WIN32)
    return()
  endif()

  file(REMOVE ${CMAKE_BINARY_DIR}/LastCoverageResults.log)

  if (EXISTS ${A_OUTPUT_DIRECTORY}/index.html)
    file(REMOVE_RECURSE ${A_OUTPUT_DIRECTORY})
  endif()

  set(ENV{OpenCppCoverage_Attach} "")
  if (A_ATTACH STREQUAL "TRUE")
    set(ENV{OpenCppCoverage_Attach} "1")
  endif()

  set(var ${A_OPENCPPCOV_PATH})
  message(STATUS "Starting ${var}")

  set(process_args ${A_CMDLINE})
  list(APPEND process_args --excluded_sources *\\crt\\*
    --excluded_sources *\\unifiedcrt\\*
    --excluded_sources *\\program*microsoft*
    --excluded_sources *\\windows*
    --excluded_sources *\\local\\pmm\\*
    --excluded_sources *\\tests_unit\\*
    --excluded_sources *\\ucrt\\*
    --excluded_sources *\\vctools\\*
    --excluded_sources *\\shared\\inc\\*
    --export_type "${A_EXPORT_TYPE}:${A_OUTPUT_DIRECTORY}"
  )

  if (NOT ${A_INPUT_COVERAGE} STREQUAL "")
    list(APPEND process_args --input_coverage ${A_INPUT_COVERAGE})
  endif()

  list(APPEND process_args --verbose)
  if (NOT "${A_EXECUTABLE_PATH}" STREQUAL "")
    list(APPEND process_args -- ${A_EXECUTABLE_PATH})
  endif()

  execute_process(COMMAND ${var} ${process_args}
    RESULT_VARIABLE res
    ERROR_VARIABLE err
    OUTPUT_VARIABLE out
  )

  set(${A_OUT_ERR} ${err} PARENT_SCOPE)
  set(${A_OUT_EXIT_CODE} ${res} PARENT_SCOPE)

  file(MAKE_DIRECTORY "${A_WORKING_DIRECTORY}")
  if (EXISTS "${A_WORKING_DIRECTORY}")
    file(WRITE ${A_WORKING_DIRECTORY}/OpenCppCoverage-stderr.log ${err})
    file(WRITE ${A_WORKING_DIRECTORY}/OpenCppCoverage-stdout.log ${out})
    file(COPY ${CMAKE_BINARY_DIR}/LastCoverageResults.log
      DESTINATION ${A_WORKING_DIRECTORY}
    )
  endif()
endfunction()


#
# skymp_execute_process.cmake:
#

function(skymp_execute_process)
  set(options)
  set(oneValueArgs EXECUTABLE_PATH OUT_EXIT_CODE OUT_STDOUT OUT_STDERR CPPCOV_PATH CPPCOV CPPCOV_OUTPUT_DIRECTORY CPPCOV_TAG ATTACH)
  set(multiValueArgs CPPCOV_CMDLINE CPPCOV_SOURCES CPPCOV_MODULES)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  if (NOT EXISTS "${A_EXECUTABLE_PATH}")
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

  if (A_ATTACH)
    set(ATTACH_OPTION ATTACH)
  else()
    set(ATTACH_OPTION NO_ATTACH)
  endif()

  if (A_ATTACH)
    if (NOT A_CPPCOV)
      set(A_CPPCOV ON)
      set(A_CPPCOV_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/cppcov_out_dummy)
      list(APPEND CPPCOV_CMDLINE --excluded_modules * --excluded_sources *)
      if (NOT A_CPPCOV_PATH)
        message(FATAL_ERROR "ATTACH mode requires CPPCOV_PATH argument regardless of CPPCOV argument")
      endif()
    endif()
  endif()

  if (A_CPPCOV)
    if (NOT A_CPPCOV_OUTPUT_DIRECTORY)
      message(FATAL_ERROR "CPPCOV_OUTPUT_DIRECTORY is not specified")
    endif()

    if (NOT "${A_CPPCOV_PATH}" STREQUAL "")
      if (NOT EXISTS ${A_CPPCOV_PATH}/OpenCppCoverage.exe)
        message(FATAL_ERROR "'${A_CPPCOV_PATH}' is not a valid path to OpenCppCoverage.exe")
      endif()
      set(A_CPPCOV_PATH "${A_CPPCOV_PATH}/OpenCppCoverage.exe")
    else()
      set(A_CPPCOV_PATH "OpenCppCoverage.exe")
    endif()

    string(RANDOM LENGTH 16 str)
    set(str "coverage_${str}.tmp")
    set(tag_dir ${CMAKE_CURRENT_BINARY_DIR}/coverage_tag_${A_CPPCOV_TAG})

    set(input_cov "${tag_dir}/coverage.tmp")
    if (NOT EXISTS "${tag_dir}/coverage.tmp")
      set(input_cov)
    endif()
    run_cppcoverage(
      ${ATTACH_OPTION}
      INPUT_COVERAGE "${input_cov}"
      OPENCPPCOV_PATH ${A_CPPCOV_PATH}
      OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${str}
      EXECUTABLE_PATH ${A_EXECUTABLE_PATH}
      EXPORT_TYPE binary
      OUT_EXIT_CODE res
      OUT_ERR err
      WORKING_DIRECTORY ${A_CPPCOV_OUTPUT_DIRECTORY}
      CMDLINE ${CPPCOV_CMDLINE}
    )
    if (NOT "${A_OUT_STDERR}" STREQUAL "")
      set(${A_OUT_STDERR} ${err} PARENT_SCOPE)
    endif()

    run_cppcoverage(
      OPENCPPCOV_PATH ${A_CPPCOV_PATH}
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


