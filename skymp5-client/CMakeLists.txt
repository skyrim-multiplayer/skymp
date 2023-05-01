project(skymp5-client)

include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

yarn_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

set(out
  ${CMAKE_BINARY_DIR}/dist/client/Data/Platform/Plugins/skymp5-client.js
)
add_custom_command(
  OUTPUT ${out}
  COMMAND yarn --cwd "\"${CMAKE_CURRENT_LIST_DIR}\"" build
  DEPENDS ${sources}
)
add_custom_target(skymp5-client ALL
  DEPENDS ${out}
  SOURCES ${sources}
)

add_dependencies(skymp5-client skyrim-platform)

add_custom_command(
  TARGET skymp5-client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -DCLIENT_SETTINGS_JSON_PATH=${CMAKE_BINARY_DIR}/dist/client/Data/Platform/Plugins/skymp5-client-settings.txt -DOFFLINE_MODE=${OFFLINE_MODE} -P ${CMAKE_SOURCE_DIR}/cmake/scripts/generate_client_settings.cmake
)
