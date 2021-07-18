include(${CMAKE_SOURCE_DIR}/cmake/npm.cmake)

# initialize npm dependencies in skymp5-server/node_modules
npm_execute_command(
  WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${SKYMP5_SERVER_SOURCE_DIR}/ts/*)

add_custom_target(skymp5-server-ts ALL
  WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
  SOURCES ${sources}
  COMMAND npm run build-ts
)