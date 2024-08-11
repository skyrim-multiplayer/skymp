project(libespm)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB ESPM_SRC "include/*.h" "src/*.cpp" "src/*.h")
add_library(espm STATIC ${ESPM_SRC})
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)

apply_default_settings(TARGETS espm)

add_library(libespm ALIAS espm)

target_link_libraries(espm PUBLIC viet)
