project(skymp5-functions-lib)

if(BUILD_GAMEMODE)
  include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

  yarn_execute_command(
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    COMMAND install
  )

  file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

  add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/dist/server/gamemode.js
    COMMAND yarn --cwd "\"${CMAKE_CURRENT_LIST_DIR}\"" build
    DEPENDS ${sources}
  )
  add_custom_target(skymp5-functions-lib ALL
    DEPENDS ${CMAKE_BINARY_DIR}/dist/server/gamemode.js
    SOURCES ${sources}
  )

  add_dependencies(skymp5-functions-lib skyrim-platform)

  add_test(
    NAME test_functions_lib
    COMMAND ${CMAKE_COMMAND}
      -DSKYMP5_FUNCTIONS_LIB_DIR=${CMAKE_CURRENT_LIST_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/run_test_functions_lib.cmake
  )
else()
  add_custom_target(skymp5-functions-lib ALL
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-functions-lib is disabled. To enable it, set BUILD_GAMEMODE to ON."
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E true > ${CMAKE_BINARY_DIR}/dist/server/gamemode.js
  )
endif()

add_dependencies(skymp5-functions-lib skyrim-platform)
