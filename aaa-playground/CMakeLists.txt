include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB_RECURSE aaa_playground_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*")# "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
add_executable(aaa_playground ${aaa_playground_sources})
target_include_directories(aaa_playground PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
apply_default_settings(TARGETS aaa_playground)


find_package(cpptrace CONFIG REQUIRED)
target_link_libraries(aaa_playground PRIVATE cpptrace::cpptrace antigo)
