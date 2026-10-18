cmake_minimum_required(VERSION 3.15)

file(MAKE_DIRECTORY "${FRONTEND_UI_DIR}")

if(EXISTS "${FRONTEND_BUILD_JS}")
  get_filename_component(FRONTEND_BUILD_DIR "${FRONTEND_BUILD_JS}" DIRECTORY)
  file(COPY "${FRONTEND_BUILD_DIR}/" DESTINATION "${FRONTEND_UI_DIR}")
  message(STATUS "Copied frontend bundle and assets from ${FRONTEND_BUILD_DIR} to ${FRONTEND_UI_DIR}")
else()
  message(STATUS "Frontend build.js not found at ${FRONTEND_BUILD_JS}, skipping UI copy. Build with BUILD_FRONT=ON or pre-build skymp5-front to include the admin UI.")
endif()
