project(skymp5-chat)

set(CHAT_UI_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src/client")
set(CHAT_UI_DEST_DIR "${CMAKE_BINARY_DIR}/dist/client/Data/Platform/UI")

if(BUILD_FRONT)
  add_custom_target(skymp5-chat-ui ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CHAT_UI_DEST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CHAT_UI_SOURCE_DIR}/index.html ${CHAT_UI_DEST_DIR}/index.html
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CHAT_UI_SOURCE_DIR}/build.js ${CHAT_UI_DEST_DIR}/build.js
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CHAT_UI_SOURCE_DIR}/style.css ${CHAT_UI_DEST_DIR}/style.css
    COMMENT "Installing skymp5-chat UI replacement..."
    VERBATIM
  )

  if(TARGET skymp5-front)
    add_dependencies(skymp5-chat-ui skymp5-front)
  endif()
else()
  add_custom_target(skymp5-chat-ui ALL
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-chat UI is disabled because BUILD_FRONT is OFF."
  )
endif()
