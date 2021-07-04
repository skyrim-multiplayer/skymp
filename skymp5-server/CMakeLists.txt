cmake_minimum_required(VERSION 3.19.1)
project(skymp5-server)

enable_testing()

add_subdirectory(cpp)
#add_subdirectory(ts)

#add_library(node INTERFACE)
#if (UNIX)
#  target_include_directories(node INTERFACE "/node-v14.13.1/include/node")
#  target_include_directories(node INTERFACE "/node-v14.13.1/include")
#  target_include_directories(node INTERFACE "/node-v14.13.1")
#else()
#  target_link_libraries(node INTERFACE ${CMAKE_JS_LIB})
#  target_include_directories(node INTERFACE ${CMAKE_JS_INC})
#  target_sources(node INTERFACE ${CMAKE_JS_SRC})
#endif()

# Include N-API wrappers
#execute_process(COMMAND node -p "require('node-addon-api').include"
#  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#  OUTPUT_VARIABLE NODE_ADDON_API_DIR
#)
#string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
#string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
#target_include_directories(node INTERFACE ${NODE_ADDON_API_DIR})

#file(GLOB_RECURSE SOURCE_FILES "${CMAKE_SOURCE_DIR}/scamp_native/addon/*")
#if (MSVC)
#  list(APPEND SOURCE_FILES ".clang-format")
#endif()
#add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})
#set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
#target_link_libraries(${PROJECT_NAME} PRIVATE node mp_common server_guest_lib)
#list(APPEND VCPKG_DEPENDENT ${PROJECT_NAME})

#include(cmake/third_party/cmake_scripts_collection.cmake)
#apply_default_settings(TARGETS ${PROJECT_NAME})

