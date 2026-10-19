include(${CMAKE_SOURCE_DIR}/cmake/package_manager.cmake)

file(GLOB_RECURSE sources ${CMAKE_CURRENT_LIST_DIR}/*.ts)

set(out
  ${CMAKE_BINARY_DIR}/dist/server/dist_back/skymp5-server.js
)

if(WIN32)
  add_custom_command(
    OUTPUT ${out}
    COMMAND npm --prefix ${SKYMP5_SERVER_SOURCE_DIR} run build-ts
    DEPENDS ${sources}
  )
else()
  add_custom_command(
    OUTPUT ${out}
    COMMAND npm ci
    COMMAND npm run build-ts
    WORKING_DIRECTORY ${SKYMP5_SERVER_SOURCE_DIR}
    DEPENDS ${sources}
  )
endif()

add_custom_target(skymp5-server-ts ALL
  DEPENDS ${out}
  SOURCES ${sources}
)
