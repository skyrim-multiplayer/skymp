function(add_papyrus_library)
  set(options)
  set(oneValueArgs NAME DIRECTORY OUTPUT_DIR)
  set(multiValueArgs)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  file(GLOB src ${A_DIRECTORY}/*.psc)

  if(WIN32)
    set(EXECUTABLE_SUFFIX ".exe")
    set(V_ARCHIVE "v_windows.zip")
    set(V_OS "windows")
  else()
    set(EXECUTABLE_SUFFIX "")
    set(V_ARCHIVE "v_linux.zip")
    set(V_OS "linux")
  endif()
  set(V_EXECUTABLE_NAME "v${EXECUTABLE_SUFFIX}")
  set(V_URL "https://github.com/vlang/v/releases/download/weekly.2021.26/${V_ARCHIVE}")

  if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/vlang/v/${V_EXECUTABLE_NAME})
    message(STATUS "Installing vlang compiler")
    make_directory(${CMAKE_CURRENT_BINARY_DIR}/vlang)
    file(DOWNLOAD ${V_URL} ${CMAKE_CURRENT_BINARY_DIR}/${V_ARCHIVE})
    file(ARCHIVE_EXTRACT
      INPUT ${CMAKE_CURRENT_BINARY_DIR}/${V_ARCHIVE} 
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/vlang
    )
  else()
    message(STATUS "Found vlang compiler")
  endif()

  set(PAPYRUS_COMMIT "598fd4c72a05eef12a67512ee0f4355b74271bbb")
  set(PAPYRUS_EXECUTABLE_PATH "${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler/papyrus-compiler-${PAPYRUS_COMMIT}/papyrus${EXECUTABLE_SUFFIX}")
  if(NOT EXISTS ${PAPYRUS_EXECUTABLE_PATH})
    message(STATUS "Building papyrus compiler")
    file(DOWNLOAD 
      "https://github.com/skyrim-multiplayer/papyrus-compiler/archive/${PAPYRUS_COMMIT}.zip"
      ${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler.zip
    )
    file(ARCHIVE_EXTRACT 
      INPUT ${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler.zip
      DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler
    )
    if(MSVC)
      set(VLANG_COMPILER msvc)
    else()
      set(VLANG_COMPILER clang)
    endif()
    execute_process(
      COMMAND "${CMAKE_CURRENT_BINARY_DIR}/vlang/v/${V_EXECUTABLE_NAME}" -cc ${VLANG_COMPILER} -m64 -os ${V_OS} -o "${PAPYRUS_EXECUTABLE_PATH}" -prod -compress -path "@vlib|@vmodules|modules" "compiler"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler/papyrus-compiler-${PAPYRUS_COMMIT}
    )
  else()
    message(STATUS "Found papyrus compiler")
  endif()

  set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/pex/${A_NAME})
  if(A_OUTPUT_DIR)
    set(OUTPUT_DIR ${A_OUTPUT_DIR})
  endif()
  add_custom_target(${A_NAME} ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${CMAKE_CURRENT_BINARY_DIR}/tmp-papyrus-builtin"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_BINARY_DIR}/papyrus-compiler/papyrus-compiler-${PAPYRUS_COMMIT}/bin" "${CMAKE_CURRENT_BINARY_DIR}/tmp-papyrus-builtin"
    COMMAND "${PAPYRUS_EXECUTABLE_PATH}" -compile -nocache -input "${A_DIRECTORY}" -output "${OUTPUT_DIR}"
    SOURCES ${src}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/tmp-papyrus-builtin"
  )
endfunction()
