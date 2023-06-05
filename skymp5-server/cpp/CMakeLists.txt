enable_testing()

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

#
# MpClientPlugin
#

if(WIN32)
  file(GLOB_RECURSE src "client/*")
  list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
  add_library(MpClientPlugin SHARED ${src})
  target_link_libraries(MpClientPlugin PUBLIC mp_common)
  set_target_properties(MpClientPlugin PROPERTIES OUTPUT_NAME "MpClientPlugin")
  apply_default_settings(TARGETS MpClientPlugin)
  list(APPEND VCPKG_DEPENDENT MpClientPlugin)

  # Not mistake that skyrim-platform depends on MpClientPlugin
  # skyrim-platform is a logical target that invokes dev_service
  # dev_service wants to access built MpClientPlugin.dll
  add_dependencies(skyrim-platform MpClientPlugin)
endif()

#
# mp_common
#

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/mp_common/*")
list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(mp_common STATIC ${src})
target_compile_definitions(mp_common PUBLIC MAX_PLAYERS=1000)
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/mp_common")
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/third_party")
target_link_libraries(mp_common PUBLIC viet)
apply_default_settings(TARGETS mp_common)
list(APPEND VCPKG_DEPENDENT mp_common)
if(WIN32)
  target_link_libraries(mp_common PUBLIC ws2_32.lib)
endif()

#
# geo
#

# https://www.codeproject.com/Articles/1065730/Point-Inside-Convex-Polygon-in-Cplusplus
file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/geo/*")
list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(geo STATIC ${src})
target_include_directories(geo PUBLIC "${CMAKE_CURRENT_LIST_DIR}/geo")
apply_default_settings(TARGETS geo)

#
# server_guest_lib
#

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/server_guest_lib/*")
list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(server_guest_lib STATIC ${src})
target_include_directories(server_guest_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/server_guest_lib")
apply_default_settings(TARGETS server_guest_lib)
list(APPEND VCPKG_DEPENDENT server_guest_lib)
target_link_libraries(server_guest_lib PUBLIC viet mp_common espm papyrus-vm-lib geo)

#
# localization-provider
#

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/localization_provider/*")
list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(localization-provider STATIC ${src})
apply_default_settings(TARGETS localization-provider)
target_include_directories(localization-provider PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/localization_provider")

#
# Link vcpkg deps
#

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/modules)

foreach(target ${VCPKG_DEPENDENT})
  find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATH_SUFFIXES catch2)
  get_filename_component(CATCH_INCLUDE_DIR ${CATCH_INCLUDE_DIR} DIRECTORY)
  target_include_directories(${target} PUBLIC ${CATCH_INCLUDE_DIR})

  find_path(SPARSEPP_INCLUDE_DIR NAMES spp.h PATH_SUFFIXES sparsepp)
  get_filename_component(SPARSEPP_INCLUDE_DIR ${SPARSEPP_INCLUDE_DIR} DIRECTORY)
  target_include_directories(${target} PUBLIC ${SPARSEPP_INCLUDE_DIR})

  find_path(JSON_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
  get_filename_component(JSON_INCLUDE_DIR ${JSON_INCLUDE_DIR} DIRECTORY)
  target_include_directories(${target} PUBLIC ${JSON_INCLUDE_DIR})

  find_package(spdlog CONFIG REQUIRED)
  target_link_libraries(${target} PUBLIC spdlog::spdlog)

  find_package(simdjson CONFIG REQUIRED)
  target_link_libraries(${target} PUBLIC simdjson::simdjson)

  find_package(slikenet CONFIG REQUIRED)
  target_link_libraries(${target} PUBLIC SLikeNet)

  find_package(ZLIB REQUIRED)
  target_link_libraries(${target} PUBLIC ZLIB::ZLIB)

  find_package(mongocxx REQUIRED)
  find_package(mongoc-1.0 CONFIG REQUIRED)
  target_link_libraries(${target} PUBLIC mongo::mongocxx_static mongo::bsoncxx_static mongo::mongoc_static mongo::bson_static)
  find_package(OpenSSL REQUIRED)
  target_link_libraries(${target} PUBLIC OpenSSL::SSL OpenSSL::Crypto)

  if(UNIX)
    target_link_libraries(${target} PUBLIC pthread resolv rt m)
  endif()
  if(WIN32)
    target_link_libraries(${target} PUBLIC Secur32.lib Crypt32.lib Dnsapi.lib Bcrypt.lib)
  endif()
endforeach()
