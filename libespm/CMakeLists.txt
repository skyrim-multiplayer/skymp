project(libespm)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)

file(GLOB ESPM_SRC "include/*.h" "src/*.cpp" "src/*.h")
add_library(espm STATIC ${ESPM_SRC})
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_include_directories(espm PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
if(UNIX)
  target_link_libraries(espm PUBLIC stdc++fs)
endif()

apply_default_settings(TARGETS espm)
link_vcpkg_dependencies(TARGETS espm)

add_library(libespm ALIAS espm)
