project(savefile)
include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB_RECURSE savefile_src "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/*.h" "${CMAKE_CURRENT_LIST_DIR}/include/*.h")

list(APPEND savefile_src ${CMAKE_SOURCE_DIR}/.clang-format)

add_library(savefile STATIC ${savefile_src})
target_include_directories(savefile PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_include_directories(savefile PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
apply_default_settings(TARGETS savefile)

find_package(ZLIB REQUIRED)
target_link_libraries(savefile PUBLIC ZLIB::ZLIB)
