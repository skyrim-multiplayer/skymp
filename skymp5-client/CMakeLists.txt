cmake_minimum_required(VERSION 3.19.1)
project(skymp5-client)

include(${CMAKE_SOURCE_DIR}/cmake/npm.cmake)

npm_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

add_custom_target(skymp5-client ALL
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  SOURCES ${sources}
  COMMAND npm run build
)
add_dependencies(skymp5-client skyrim-platform)

# The server requires `dist/server/dist_front` to contain `skymp5-client.js`
# This is to be able to reload `skymp5-client.js` for all users when changed
# That feature is deprecated and would be removed

add_custom_command(
  TARGET skymp5-client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/dist/client/Data/Platform/Plugins/skymp5-client.js ${CMAKE_BINARY_DIR}/dist/server/dist_front/skymp5-client.js
)