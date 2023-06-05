project(savefile)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB_RECURSE savefile_src "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

list(APPEND savefile_src ${CMAKE_SOURCE_DIR}/.clang-format)

add_library(savefile STATIC savefile_src)
target_include_directories(savefile PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
apply_default_settings(savefile)
