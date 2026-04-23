project(skymp5-gamemode)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*.ts)

set(out ${CMAKE_BINARY_DIR}/dist/server/gamemode.js)

if(WIN32)
  add_custom_command(
    OUTPUT ${out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND ${CMAKE_COMMAND} -E env "GAMEMODE_OUTPUT_DIR=${CMAKE_BINARY_DIR}/dist/server"
            yarn --cwd "${CMAKE_CURRENT_LIST_DIR}" build
    DEPENDS ${sources}
  )
else()
  add_custom_command(
    OUTPUT ${out}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/server
    COMMAND yarn
    COMMAND ${CMAKE_COMMAND} -E env "GAMEMODE_OUTPUT_DIR=${CMAKE_BINARY_DIR}/dist/server"
            yarn build
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    DEPENDS ${sources}
  )
endif()

add_custom_target(skymp5-gamemode ALL
  DEPENDS ${out}
  SOURCES ${sources}
)
