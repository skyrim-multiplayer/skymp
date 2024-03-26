include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

# initialize npm dependencies in skymp5-server/node_modules
yarn_execute_command(
  WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/*.ts)

set(out
  ${CMAKE_BINARY_DIR}/dist/server/dist_back/skymp5-server.js
)
add_custom_command(
  OUTPUT ${out}
  COMMAND yarn --cwd "\"${SKYMP5_SERVER_SOURCE_DIR}\"" build-ts
  DEPENDS ${sources}
)
add_custom_target(skymp5-server-ts ALL
  DEPENDS ${out}
  SOURCES ${sources}
)
