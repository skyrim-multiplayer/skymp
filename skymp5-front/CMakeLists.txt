project(skymp5-front)

set(FRONTEND_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(FRONTEND_BUILD_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/build-local.cmake")
set(FRONTEND_JS_DEST_DIR "${CMAKE_BINARY_DIR}/dist/client/Data/Platform/UI")

if(BUILD_FRONT)
  add_custom_target(skymp5-front ALL
    SOURCES ${FRONTEND_BUILD_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/dist/client
    COMMAND ${CMAKE_COMMAND}
            -DFRONTEND_SOURCE_DIR=${FRONTEND_SOURCE_DIR}
            -DFRONTEND_JS_DEST_DIR=${FRONTEND_JS_DEST_DIR}
            -DYARN_HELPERS_PATH=${CMAKE_SOURCE_DIR}/cmake/yarn.cmake
            -P ${FRONTEND_BUILD_SCRIPT}
    COMMENT "Building skymp5-front from local sources..."
    VERBATIM
  )
else()
  add_custom_target(skymp5-front ALL
    SOURCES ${FRONTEND_BUILD_SCRIPT}
    COMMAND ${CMAKE_COMMAND} -E echo "Building skymp5-front is disabled. To enable it, set BUILD_FRONT to ON."
  )
endif()
