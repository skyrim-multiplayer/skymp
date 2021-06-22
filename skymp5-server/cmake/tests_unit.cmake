enable_testing()

add_test(
  NAME test_unit
  COMMAND ${CMAKE_COMMAND}
    -DEXE_PATH=$<TARGET_FILE:unit>
    -DCOVERAGE_HTML_OUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/__coverage
    -DOPENCPPCOV_DIR=${OPENCPPCOV_DIR}
    -DCPPCOV=${CPPCOV}
    -P ${CMAKE_CURRENT_LIST_DIR}/run_test_unit.cmake
)
