project(serialization)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)

file(GLOB serialization_SRC "include/*.h" "src/*.cpp" "src/*.h")
add_library(serialization STATIC ${serialization_SRC})
target_include_directories(serialization PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(serialization PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

apply_default_settings(TARGETS serialization)
link_vcpkg_dependencies(TARGETS serialization)
