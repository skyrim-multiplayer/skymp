include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)

file(GLOB_RECURSE viet_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
add_library(viet STATIC ${viet_sources})
target_include_directories(viet PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(viet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
apply_default_settings(TARGETS viet)
link_vcpkg_dependencies(TARGETS viet)
