project(client-deps)

if(WIN32)
  if(SKYRIM_SE)
    set(CLIENT_DEPS_GAME se)
  else()
    set(CLIENT_DEPS_GAME ae)
  endif()

  add_custom_target(client-deps ALL
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    SOURCES ${sources}
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/common/Data" "${CMAKE_BINARY_DIR}/dist/client/data"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_LIST_DIR}/${CLIENT_DEPS_GAME}/Data" "${CMAKE_BINARY_DIR}/dist/client/data"
  )

  add_dependencies(client-deps skyrim-platform)
else()
  add_custom_target(client-deps ALL SOURCES ${sources} 
    COMMAND ${CMAKE_COMMAND} -E echo "Building client-deps is disabled on non-Windows setups."
  )
endif()
