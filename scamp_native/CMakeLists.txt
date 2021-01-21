include(${CMAKE_SOURCE_DIR}/cmake/third_party/pmm.cmake)
# Clear CMake cache after changing this
# (also for subprojects like platform_se)
set(VCPKG_REVISION 790910f79f653978f90aadd958abf3c407215552)
set(triplet x64-windows-static)

if (UNIX)
  set(VCPKG_PATH "/vcpkg")
  if (NOT EXISTS "${VCPKG_PATH}/bootstrap-vcpkg.sh")
    message(FATAL_ERROR "VCPKG_PATH ('${VCPKG_PATH}') is not a valid vcpkg path")
  endif()
else()
  # Download vcpkg dependencies
  set(ports
    catch2:${triplet}
    sparsepp:${triplet}
    nlohmann-json:${triplet}
    spdlog:${triplet}
    slikenet:${triplet}
    boost-bimap:${triplet}
    zlib:${triplet}
    sqlite-orm:${triplet}
    sqlpp11:${triplet}
    sqlpp11-connector-sqlite3:${triplet}
    mongo-cxx-driver:${triplet} # Doesn't build on Windows without this https://github.com/microsoft/vcpkg/pull/14825
  )
  set(VCPKG_PATH ${_VCPKG_ROOT_DIR})
endif()

if (WIN32)
  list(APPEND ports simdjson:${triplet})
endif()
foreach(port ${ports})
  message(STATUS "Installing ${port}")
  pmm(VCPKG REVISION ${VCPKG_REVISION} REQUIRES ${port})
endforeach()
if ("${VCPKG_PATH}" STREQUAL "")
  message(FATAL_ERROR "Expected VCPKG_PATH variable to be set")
endif()

# TODO: Check SHA512
include(${CMAKE_SOURCE_DIR}/cmake/third_party/cmake_scripts_collection.cmake)

if (NOT CPPCOV)
  message(WARNING "CPPCOV is not set, coverage for C++ code is disabled")
endif()

if(WIN32)
  option(SKYRIM_DIR "Path to Skyrim" OFF)
  if (NOT SKYRIM_DIR)
    message(WARNING "SKYRIM_DIR is not specified, MpClientPlugin will not be installed")
  endif()
  if (SKYRIM_DIR AND NOT EXISTS ${SKYRIM_DIR}/SkyrimSE.exe)
    message(FATAL_ERROR "Bad SKYRIM_DIR: ${SKYRIM_DIR}")
  endif()

  file(GLOB_RECURSE src "client/*" "${CMAKE_SOURCE_DIR}/.clang-format")
  add_library(MpClientPlugin SHARED ${src})
  target_link_libraries(MpClientPlugin PUBLIC mp_common)
  set_target_properties(MpClientPlugin PROPERTIES OUTPUT_NAME "MpClientPlugin")
  apply_default_settings(TARGETS MpClientPlugin)
  list(APPEND VCPKG_DEPENDENT MpClientPlugin)

  if(SKYRIM_DIR)
    add_custom_command(
      TARGET MpClientPlugin POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:MpClientPlugin> ${SKYRIM_DIR}/Data/SKSE/plugins
    )
  endif()
endif()

set(OPENCPPCOV_DIR "")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib_espm)
list(APPEND VCPKG_DEPENDENT espm)
apply_default_settings(TARGETS espm)

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/unit/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_executable(unit ${src})
target_link_libraries(unit PRIVATE server_guest_lib)
target_include_directories(unit PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/mp_common")
apply_default_settings(TARGETS unit)
list(APPEND VCPKG_DEPENDENT unit)
target_compile_definitions(unit PRIVATE
  TEST_PEX_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files/standard_scripts\"
  BUILT_PEX_DIR=\"${CMAKE_CURRENT_BINARY_DIR}/pex\"
)

if (WIN32)
  target_compile_options(unit PRIVATE "/bigobj")
endif()

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/mp_common/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(mp_common STATIC ${src})
target_compile_definitions(mp_common PUBLIC MAX_PLAYERS=1000)
if (SKYMP_DANGER_ENABLE_PARTNER_FEATURES)
  target_compile_definitions(mp_common PUBLIC SKYMP_PARTNER=1)
  message(STATUS "Selected Partner mode")
  message(WARNING "Hope you know what you do. Do not distribute binaries!")
else()
  target_compile_definitions(mp_common PUBLIC SKYMP_LITE=1)
  message(STATUS "Selected Lite mode")
endif()
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/mp_common")
target_include_directories(mp_common PUBLIC "${CMAKE_CURRENT_LIST_DIR}/third_party")
apply_default_settings(TARGETS mp_common)
list(APPEND VCPKG_DEPENDENT mp_common)
if (WIN32)
  target_link_libraries(mp_common PUBLIC ws2_32.lib)
endif()

# https://www.codeproject.com/Articles/1065730/Point-Inside-Convex-Polygon-in-Cplusplus
file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/geo/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(geo STATIC ${src})
target_include_directories(geo PUBLIC "${CMAKE_CURRENT_LIST_DIR}/geo")
apply_default_settings(TARGETS geo)

file(GLOB_RECURSE src "${CMAKE_CURRENT_SOURCE_DIR}/server_guest_lib/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(server_guest_lib STATIC ${src})
target_include_directories(server_guest_lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/server_guest_lib")
apply_default_settings(TARGETS server_guest_lib)
list(APPEND VCPKG_DEPENDENT server_guest_lib)
target_link_libraries(server_guest_lib PUBLIC mp_common espm papyrus-vm-lib geo)
if (WIN32)
  file(COPY
    ${CMAKE_SOURCE_DIR}/cmake/ctest.bat
    ${CMAKE_SOURCE_DIR}/cmake/before_ctest.cmake
    DESTINATION ${CMAKE_BINARY_DIR}
  )
  file(APPEND ${CMAKE_BINARY_DIR}/before_ctest.cmake
    "\n\n# This file was copied from cmake/before_ctest.cmake, do not modify"
  )
endif()

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/papyrus_vm_lib/*" "${CMAKE_SOURCE_DIR}/.clang-format")
add_library(papyrus-vm-lib STATIC ${src})
apply_default_settings(TARGETS papyrus-vm-lib)
target_include_directories(papyrus-vm-lib PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/papyrus_vm_lib")
target_include_directories(papyrus-vm-lib PUBLIC "${CMAKE_CURRENT_LIST_DIR}/third_party")
if (UNIX)
  target_link_libraries(papyrus-vm-lib PUBLIC stdc++fs)
endif()

if (NOT SKYRIM_DIR)
  message(WARNING "SKYRIM_DIR is not specified, Papyrus sources will not be built")
else()
  include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library.cmake)
  add_papyrus_library(NAME papyrus_test_files DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/tests_unit.cmake)

if(UNIX)
  # On Linux we use CMAKE_TOOLCHAIN_FILE, see root CMakeLists.txt

  foreach(target ${VCPKG_DEPENDENT})
    find_package(Catch2 CONFIG REQUIRED)
    target_link_libraries(${target} PUBLIC Catch2::Catch2)

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

    find_package(Boost REQUIRED)
    target_include_directories(${target} PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(${target} PUBLIC ${Boost_LIBRARIES})

    find_package(ZLIB REQUIRED)
    target_link_libraries(${target} PUBLIC ZLIB::ZLIB)

    find_path(SQLITEORM_INCLUDE_DIR NAMES sqlite_orm.h REQUIRED)
    target_include_directories(${target} PUBLIC ${SQLITEORM_INCLUDE_DIR})

    find_package(unofficial-sqlite3 CONFIG REQUIRED)
    target_link_libraries(${target} PUBLIC unofficial::sqlite3::sqlite3)

    find_package(libmongocxx REQUIRED)
    find_package(libbsoncxx REQUIRED)
    find_package(libmongoc-1.0 CONFIG REQUIRED)
    target_link_libraries(${target} PUBLIC ${LIBMONGOCXX_LIBRARIES} ${LIBBSONCXX_LIBRARIES} ${MONGOC_LIBRARIES} ${MONGOC_STATIC_LIBRARIES})
    find_package(OpenSSL REQUIRED)
    target_link_libraries(${target} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_link_libraries(${target} PUBLIC pthread resolv rt m) # sasl2 ?

  endforeach()

else()
  integrate_vcpkg(
    TARGETS ${VCPKG_DEPENDENT}
    VCPKG_PATH "${VCPKG_PATH}"
  )
endif()

if(UNIX)
  foreach(target ${VCPKG_DEPENDENT})
    target_link_libraries(${target} PUBLIC pthread)
  endforeach()
endif()

if(WIN32)
  foreach(target ${VCPKG_DEPENDENT})
    target_link_libraries(${target} PUBLIC Secur32.lib Crypt32.lib Dnsapi.lib Bcrypt.lib)
  endforeach()
endif()
