project(skymp5-patches)

if(BUILD_PATCHES)
  if(NOT GITHUB_TOKEN)
    message(FATAL_ERROR "GITHUB_TOKEN is not set. Please set it to your GitHub personal access token.")
  endif()

  add_custom_target(skymp5-patches ALL SOURCES ${sources} 
    COMMAND ${CMAKE_COMMAND} -E echo "WIP"
  )
else()
  add_custom_target(skymp5-patches ALL SOURCES ${sources} 
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-patches is disabled. To enable it, set BUILD_PATCHES to ON."
  )
endif()
