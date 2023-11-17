project(skymp5-downloader)

if(WIN32)
  include(${CMAKE_SOURCE_DIR}/cmake/link_vcpkg_dependencies.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/apply_default_settings.cmake)

  # Define source files
  set(sources
    src/main.cpp
    src/icon.ico
    src/resource.rc
  )

  add_executable(${PROJECT_NAME} WIN32 ${sources})
  set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\"")

  link_vcpkg_dependencies(TARGETS ${PROJECT_NAME})
  apply_default_settings(TARGETS ${PROJECT_NAME})

  target_link_libraries(${PROJECT_NAME} PRIVATE ComCtl32)
else()
  add_custom_target(${PROJECT_NAME} ALL SOURCES ${sources} 
    COMMAND ${CMAKE_COMMAND} -E echo "Building ${PROJECT_NAME} is disabled on non-Windows setups."
  )
endif()
