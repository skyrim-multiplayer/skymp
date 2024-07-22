include(${CMAKE_SOURCE_DIR}/cmake/yarn.cmake)

yarn_execute_command(
  COMMAND install
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/dependencies
)

add_test(
  NAME redis_test
  COMMAND node ${CMAKE_CURRENT_SOURCE_DIR}/redis_test/test_build_redis_cache.js
)
