if(WITH_ANTIGO)
  file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/src/*")
  list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
  if(TARGET platform_lib)
    file(GLOB src_windows "${CMAKE_CURRENT_SOURCE_DIR}/platform_lib_tests/*")
    list(APPEND src ${src_windows})
  endif()

  add_executable(antigo_unit ${src})

  find_package(Catch2 CONFIG REQUIRED)
  target_link_libraries(antigo_unit PRIVATE Catch2::Catch2)

  target_link_libraries(antigo_unit PUBLIC antigo)
  apply_default_settings(TARGETS antigo_unit)
  list(APPEND VCPKG_DEPENDENT antigo_unit)

  if(WIN32)
    target_compile_options(antigo_unit PRIVATE "/bigobj")
    target_link_libraries(antigo_unit PUBLIC Dbghelp.lib)
  endif()

  #
  # ctest tests
  #

  # run without coverage but with dumps
  add_test(
    NAME test_unit
    COMMAND ${CMAKE_COMMAND}
      -DEXE_PATH=$<TARGET_FILE:antigo_unit>
      -DCOVERAGE_HTML_OUT_DIR=${CMAKE_BINARY_DIR}/__coverage
      -DCPPCOV=OFF
      -DCPPCOV_PATH=${CPPCOV_PATH}
      -DUNIT_WORKING_DIRECTORY=${CMAKE_BINARY_DIR}
      -P ${CMAKE_SOURCE_DIR}/cmake/run_test_unit.cmake
  )
endif()
