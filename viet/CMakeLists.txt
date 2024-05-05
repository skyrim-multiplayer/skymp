include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB_RECURSE viet_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
add_library(viet STATIC ${viet_sources})
target_include_directories(viet PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(viet PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
apply_default_settings(TARGETS viet)

find_path(MAKEID_INCLUDE_DIR NAMES MakeID.h)
target_include_directories(viet PUBLIC ${MAKEID_INCLUDE_DIR})

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(viet PUBLIC spdlog::spdlog)
