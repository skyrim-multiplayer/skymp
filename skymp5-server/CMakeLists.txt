cmake_minimum_required(VERSION 3.19.1)
project(skymp5-server)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

enable_testing()

add_subdirectory(cpp)
add_subdirectory(ts)

# Generate node.cmake
npm_execute_command(
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  COMMAND run cmake-js-fetch
)

# Use node.cmake to add "node" target
include(${CMAKE_CURRENT_LIST_DIR}/cmake-js-fetch-build/node.cmake)

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(node INTERFACE ${NODE_ADDON_API_DIR})

file(GLOB_RECURSE sources "${PROJECT_SOURCE_DIR}/cpp/addon/*")
if(MSVC)
  list(APPEND sources "${CMAKE_SOURCE_DIR}/.clang-format")
endif()
add_library(${PROJECT_NAME} SHARED ${sources})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} PRIVATE node mp_common server_guest_lib)
list(APPEND VCPKG_DEPENDENT ${PROJECT_NAME})
apply_default_settings(TARGETS ${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES 
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/dist/server"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/dist/server"
)