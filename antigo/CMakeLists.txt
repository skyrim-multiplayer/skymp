include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

if(WITH_ANTIGO OR DEFINED ENV{DEPLOY_ACTION})
  message(STATUS "using real antigo")

  file(GLOB_RECURSE antigo_sources "${CMAKE_CURRENT_SOURCE_DIR}/src/*" "${CMAKE_CURRENT_SOURCE_DIR}/include/*")
  add_library(antigo STATIC ${antigo_sources})
  target_include_directories(antigo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
  target_include_directories(antigo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
  apply_default_settings(TARGETS antigo)

  find_package(cpptrace CONFIG REQUIRED)
  target_link_libraries(antigo PRIVATE cpptrace::cpptrace)

  # XXX remove me
  find_package(spdlog CONFIG REQUIRED)
  target_link_libraries(antigo PUBLIC spdlog::spdlog)

  add_subdirectory(unit)
else()
  message(STATUS "using antigo STUB")

  file(GLOB_RECURSE antigo_sources "${CMAKE_CURRENT_SOURCE_DIR}/src_stub/*" "${CMAKE_CURRENT_SOURCE_DIR}/include_stub/*")
  add_library(antigo STATIC ${antigo_sources})
  target_include_directories(antigo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src_stub")
  target_include_directories(antigo PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include_stub")
  apply_default_settings(TARGETS antigo)

  set_target_properties(antigo PROPERTIES LINKER_LANGUAGE CXX)
endif()
