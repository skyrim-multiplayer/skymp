project(skymp5-client)

include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

yarn_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND install
)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/src/*)

add_custom_target(skymp5-client ALL
  SOURCES ${sources}
  COMMAND yarn --cwd "\"${CMAKE_CURRENT_LIST_DIR}\"" build
)
add_dependencies(skymp5-client skyrim-platform)

set(skymp5_client_settings_txt "{
  \"server-ip\": \"127.0.0.1\",
  \"server-port\": 7777,
  \"lobbyLocation\": {
    \"pos\": [133710, -61252, 14605],
    \"rot\": [0, 0, 0],
    \"worldOrCell\": 60
  },
  \"gameData\": {
    \"profileId\": 1
  }
}
")
file(WRITE ${PROJECT_BINARY_DIR}/skymp5-client-settings.txt.tmp "${skymp5_client_settings_txt}")
add_custom_command(
  TARGET skymp5-client POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/skymp5-client-settings.txt.tmp ${CMAKE_BINARY_DIR}/dist/client/Data/Platform/Plugins/skymp5-client-settings.txt
)
