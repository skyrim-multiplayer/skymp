project(serialization)

include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB serialization_SRC "include/*.h" "src/*.cpp" "src/*.h")
add_library(serialization STATIC ${serialization_SRC})
target_include_directories(serialization PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(serialization PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

apply_default_settings(TARGETS serialization)

find_package(slikenet CONFIG REQUIRED)
target_link_libraries(serialization PUBLIC SLikeNet)

find_package(simdjson CONFIG REQUIRED)
target_link_libraries(serialization PUBLIC simdjson::simdjson)

find_path(JSON_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
get_filename_component(JSON_INCLUDE_DIR ${JSON_INCLUDE_DIR} DIRECTORY)
target_include_directories(serialization PUBLIC ${JSON_INCLUDE_DIR})
