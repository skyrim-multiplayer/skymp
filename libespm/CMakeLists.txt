project(libespm)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB ESPM_SRC "include/*.h" "src/*.cpp" "src/*.h")
add_library(espm STATIC ${ESPM_SRC})
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

apply_default_settings(TARGETS espm)

add_library(libespm ALIAS espm)

find_path(SPARSEPP_INCLUDE_DIR NAMES spp.h PATH_SUFFIXES sparsepp)
get_filename_component(SPARSEPP_INCLUDE_DIR ${SPARSEPP_INCLUDE_DIR} DIRECTORY)
target_include_directories(espm PUBLIC ${SPARSEPP_INCLUDE_DIR})
