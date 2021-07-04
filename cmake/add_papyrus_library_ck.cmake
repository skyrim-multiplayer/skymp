function(add_papyrus_library_ck)
    set(options)
    set(oneValueArgs NAME DIRECTORY COMPILER_EXECUTABLE_PATH OUTPUT_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(A
      "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    file(GLOB src ${A_DIRECTORY}/*.psc)

    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/pex/${A_NAME})
    if(A_OUTPUT_DIR)
      set(OUTPUT_DIR ${A_OUTPUT_DIR})
    endif()
    if(EXISTS "${A_COMPILER_EXECUTABLE_PATH}")
      add_custom_target(${A_NAME} ALL
        COMMAND "${A_COMPILER_EXECUTABLE_PATH}"
        ${A_DIRECTORY} -output=${OUTPUT_DIR} -import=${A_DIRECTORY} -all
        SOURCES ${src}
      )
    else()
      message(WARNING "Missing Papyrus Compiler at ${A_COMPILER_EXECUTABLE_PATH}. Target ${A_NAME} wouldn't be built")
    endif()
  endfunction()