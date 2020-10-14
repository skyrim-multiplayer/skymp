if (POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if (UNIX)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

if (NOT MSVC)
  cmake_minimum_required(VERSION 3.14.5)
else()
  cmake_minimum_required(VERSION 3.17.0)
endif()
project(scamp_native)
enable_testing()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if (UNIX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive")
endif()

add_subdirectory(scamp_native)

add_library(node INTERFACE)
target_link_libraries(node INTERFACE ${CMAKE_JS_LIB})
target_include_directories(node INTERFACE ${CMAKE_JS_INC})
target_sources(node INTERFACE ${CMAKE_JS_SRC})

# Include N-API wrappers
execute_process(COMMAND node -p "require('node-addon-api').include"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE NODE_ADDON_API_DIR
)
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(node INTERFACE ${NODE_ADDON_API_DIR})

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_SOURCE_DIR}/scamp_native/addon/*")
if (MSVC)
  list(APPEND SOURCE_FILES ".clang-format")
endif()
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} PRIVATE node mp_common server_guest_lib)

include(cmake/third_party/cmake_scripts_collection.cmake)
apply_default_settings(TARGETS ${PROJECT_NAME})
