include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

file(GLOB_RECURSE antigo_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
add_library(antigo STATIC ${antigo_sources})
target_include_directories(antigo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(antigo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
apply_default_settings(TARGETS antigo)

# XXX probably can be removed once the first .cpp file is here
# set_target_properties(antigo PROPERTIES LINKER_LANGUAGE CXX)

# set_target_properties(antigo PROPERTIES CXX_STANDARD 23)

# target_compile_definitions(antigo PUBLIC HAVE_STACKTRACE)

# find_path(MAKEID_INCLUDE_DIR NAMES MakeID.h)
# target_include_directories(antigo PUBLIC ${MAKEID_INCLUDE_DIR})

find_package(cpptrace CONFIG REQUIRED)
target_link_libraries(antigo PRIVATE cpptrace::cpptrace)
