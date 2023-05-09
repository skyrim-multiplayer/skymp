include(${CMAKE_SOURCE_DIR}/cmake/add_papyrus_library_ck.cmake)

#
# papyrus_test_files
#

add_papyrus_library_ck(
  NAME papyrus_test_files 
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files
  OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files/pex
  COMPILER_EXECUTABLE_PATH "${SKYRIM_DIR}/Papyrus compiler/PapyrusCompiler.exe"
)

#
# unit executable
#

file(GLOB src "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(APPEND src "${CMAKE_SOURCE_DIR}/.clang-format")
if(TARGET platform_lib)
  file(GLOB src_windows "${CMAKE_CURRENT_SOURCE_DIR}/platform_lib_tests/*")
  list(APPEND src ${src_windows})
endif()

add_executable(unit ${src})

find_package(Catch2 CONFIG REQUIRED)
target_link_libraries(unit PRIVATE Catch2::Catch2)

target_link_libraries(unit PUBLIC server_guest_lib espm)
if(TARGET platform_lib)
  target_link_libraries(unit PUBLIC platform_lib)
endif()
apply_default_settings(TARGETS unit)
list(APPEND VCPKG_DEPENDENT unit)

if(SKYRIM_SE)
  target_compile_definitions(unit PRIVATE SKYRIM_SE=1)
endif()

target_compile_definitions(unit PRIVATE
  TEST_PEX_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files/standard_scripts\"
  BUILT_PEX_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/papyrus_test_files/pex\"
  SKYRIM_DIR=\"${SKYRIM_DIR}\"
  UNIT_DATA_DIR=\"${UNIT_DATA_DIR}\"
  DIST_DIR=\"${CMAKE_BINARY_DIR}/dist\"
)

if(WIN32)
  target_compile_options(unit PRIVATE "/bigobj")
  target_link_libraries(unit PUBLIC Dbghelp.lib)
endif()

#
# ctest tests
#

# run without coverage but with dumps
add_test(
  NAME test_unit
  COMMAND ${CMAKE_COMMAND}
    -DEXE_PATH=$<TARGET_FILE:unit>
    -DCOVERAGE_HTML_OUT_DIR=${CMAKE_BINARY_DIR}/__coverage
    -DCPPCOV=OFF
    -DCPPCOV_PATH=${CPPCOV_PATH}
    -DUNIT_WORKING_DIRECTORY=${CMAKE_BINARY_DIR}
    -P ${CMAKE_SOURCE_DIR}/cmake/run_test_unit.cmake
)

# run with coverage but without dumps
if(CPPCOV_PATH)
  add_test(
    NAME test_unit_coverage
    COMMAND ${CMAKE_COMMAND}
      -DEXE_PATH=$<TARGET_FILE:unit>
      -DCOVERAGE_HTML_OUT_DIR=${CMAKE_BINARY_DIR}/__coverage
      -DCPPCOV=ON
      -DCPPCOV_PATH=${CPPCOV_PATH}
      -P ${CMAKE_SOURCE_DIR}/cmake/run_test_unit.cmake
  )
endif()
