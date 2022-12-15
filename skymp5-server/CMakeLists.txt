project(skymp5-server)
enable_testing()

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

set(SKYMP5_SERVER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

# Add C++ part of the server (except cpp/addon)
add_subdirectory(cpp)

# Add TypeScript part of the server
add_subdirectory(ts)

# Include `node-addon-api`, a wrapper over NAPI.
execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

# Create target called "node" to be linked into addon
add_library(node INTERFACE)
target_include_directories(node INTERFACE ${NODE_ADDON_API_DIR})
find_package(unofficial-node-api CONFIG REQUIRED)
target_link_libraries(node INTERFACE unofficial::node-api::node-api)

file(GLOB_RECURSE sources "${SKYMP5_SERVER_SOURCE_DIR}/cpp/addon/*")
if(MSVC)
  list(APPEND sources "${CMAKE_SOURCE_DIR}/.clang-format")
endif()
add_library(${PROJECT_NAME} SHARED ${sources})

# Apply NodeJS convention (no .dll/.so, just .node)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")

# Legacy TypeScript part expects this module to be called `scam_native`
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "scam_native")

# Put build artifacts into build/dist/server
if(WIN32)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/dist/server"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/dist/server"
  )
else()
  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_BINARY_DIR}/dist/server/$<TARGET_FILE_NAME:${PROJECT_NAME}>
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE node mp_common server_guest_lib localization-provider)
list(APPEND VCPKG_DEPENDENT ${PROJECT_NAME})
apply_default_settings(TARGETS ${PROJECT_NAME})

# All targets are ready

# On Windows create launch_server.bat in build/dist/server
# This .bat file is for launching the server
if(WIN32)
  set(bat "node dist_back/skymp5-server.js\n")
  set(bat "${bat}pause\n")

  file(WRITE ${PROJECT_BINARY_DIR}/launch_server.bat.tmp "${bat}")

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/launch_server.bat.tmp ${CMAKE_BINARY_DIR}/dist/server/launch_server.bat
  )
endif()

# Create server-settings.json in build/dist/server

if(SKYRIM_DIR)
  set(esm_prefix "${SKYRIM_DIR}/Data/")
else()
  set(esm_prefix "")
endif()

message(STATUS "esm_prefix is '${esm_prefix}'")

set(server_settings_json "{
  \"dataDir\": \"data\",
  \"loadOrder\": [
    \"${esm_prefix}Skyrim.esm\",
    \"${esm_prefix}Update.esm\",
    \"${esm_prefix}Dawnguard.esm\",
    \"${esm_prefix}HearthFires.esm\",
    \"${esm_prefix}Dragonborn.esm\"
  ],
  \"ip\": \"127.0.0.1\",
  \"name\": \"My Server\",
  \"offlineMode\": true
}
")

set(server_settings_json_sweetpie "{
  \"dataDir\": \"data\",
  \"loadOrder\": [
    \"${esm_prefix}Skyrim.esm\",
    \"${esm_prefix}Update.esm\",
    \"${esm_prefix}Dawnguard.esm\",
    \"${esm_prefix}HearthFires.esm\",
    \"${esm_prefix}Dragonborn.esm\",
    \"${esm_prefix}RavenOnMyShoulder.esp\",
    \"${esm_prefix}SweetPie.esp\"
  ],
  \"name\": \"SweetPie\",
  \"startPoints\": [
    {
      \"pos\": [ 18522.08, 10218.17, 624.46 ],
      \"worldOrCell\": \"0x6042b5f\",
      \"angleZ\": 0
    }
  ]
}
")

if(SWEETPIE)
  file(WRITE ${PROJECT_BINARY_DIR}/server-settings.json.tmp "${server_settings_json_sweetpie}")
else()
  file(WRITE ${PROJECT_BINARY_DIR}/server-settings.json.tmp "${server_settings_json}")
endif()

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/server-settings.json.tmp ${CMAKE_BINARY_DIR}/dist/server/server-settings.json
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/package.json ${CMAKE_BINARY_DIR}/dist/server/package.json
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/yarn.lock ${CMAKE_BINARY_DIR}/dist/server/yarn.lock
)
