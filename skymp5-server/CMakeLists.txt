project(skymp5-server)
enable_testing()

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

set(SKYMP5_SERVER_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR})

yarn_execute_command(
  COMMAND install
  WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
)

# Find node-addon-api
find_package(unofficial-node-addon-api REQUIRED)

# Add C++ part of the server (except cpp/addon)
add_subdirectory(cpp)

# Add TypeScript part of the server
add_subdirectory(ts)

# Add C++ addon for NodeJS
file(GLOB_RECURSE sources "${SKYMP5_SERVER_SOURCE_DIR}/cpp/addon/*")
if(MSVC)
  list(APPEND sources "${CMAKE_SOURCE_DIR}/.clang-format")
endif()
add_library(${PROJECT_NAME} SHARED ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "scam_native" PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::node-addon-api::node-addon-api)
target_include_directories(${PROJECT_NAME} PRIVATE ${SKYMP5_SERVER_SOURCE_DIR}/cpp/addon)

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

target_link_libraries(${PROJECT_NAME} PRIVATE mp_common server_guest_lib localization-provider)
list(APPEND VCPKG_DEPENDENT ${PROJECT_NAME})
apply_default_settings(TARGETS ${PROJECT_NAME})

# All targets are ready

if(WIN32)
  # On Windows create launch_server.bat in ./build/dist/server
  # This .bat file is for launching the server
  set(bat "node dist_back/skymp5-server.js\n")
  set(bat "${bat}pause\n")
  file(WRITE ${PROJECT_BINARY_DIR}/launch_server.bat.tmp "${bat}")

  # Same but for dev (no pause, placed into ./build dir)
  set(bat "setlocal\n") # setlocal/endlocal is for temporary cd
  set(bat "${bat}cd dist/server\n")
  set(bat "${bat}node dist_back/skymp5-server.js\n")
  set(bat "${bat}endlocal\n")
  file(WRITE ${CMAKE_BINARY_DIR}/launch_server.bat "${bat}")

  add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_BINARY_DIR}/launch_server.bat.tmp ${CMAKE_BINARY_DIR}/dist/server/launch_server.bat
  )
endif()

# Create server-settings.json in build/dist/server

if(SKYRIM_DIR)
  set(esm_prefix "${SKYRIM_DIR}/Data/")
elseif(UNIT_DATA_DIR)
  set(esm_prefix "${UNIT_DATA_DIR}/")
else()
  set(esm_prefix "")
endif()

message(STATUS "esm_prefix is '${esm_prefix}'")

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -DESM_PREFIX=${esm_prefix} -DSERVER_SETTINGS_JSON_PATH=${CMAKE_BINARY_DIR}/dist/server/server-settings.json -DOFFLINE_MODE=${OFFLINE_MODE} -P ${CMAKE_SOURCE_DIR}/cmake/scripts/generate_server_settings.cmake
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/package.json ${CMAKE_BINARY_DIR}/dist/server/package.json
)

add_custom_command(
  TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${SKYMP5_SERVER_SOURCE_DIR}/yarn.lock ${CMAKE_BINARY_DIR}/dist/server/yarn.lock
)

file(GLOB_RECURSE standard_scripts
  ${CMAKE_CURRENT_SOURCE_DIR}/standard_scripts/*
)
cmrc_add_resource_library(server_standard_scripts ${standard_scripts}
  NAMESPACE server_standard_scripts
)
apply_default_settings(TARGETS server_standard_scripts)
target_link_libraries(server_guest_lib PUBLIC server_standard_scripts)
