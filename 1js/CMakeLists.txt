project(1js)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)

file(GLOB_RECURSE 1js_SOURCES *.cpp *.h)

if(APPLE)
    list(FILTER 1js_SOURCES EXCLUDE REGEX "private\/backends\/Chakra.*")
endif()

add_library(1js STATIC ${1js_SOURCES})
target_include_directories(1js PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
apply_default_settings(TARGETS 1js)
link_vcpkg_dependencies(TARGETS 1js)
target_link_libraries(1js PUBLIC viet)

find_package(unofficial-node-api REQUIRED)
target_link_libraries(1js PUBLIC unofficial::node-api::node-api)