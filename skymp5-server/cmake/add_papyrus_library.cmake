function(add_papyrus_library)
  set(options)
  set(oneValueArgs NAME DIRECTORY)
  set(multiValueArgs)
  cmake_parse_arguments(A
    "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
  )

  file(GLOB src ${A_DIRECTORY}/*.psc)

  set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/pex)
  add_custom_target(${A_NAME} ALL
    COMMAND "${SKYRIM_DIR}\\Papyrus Compiler\\PapyrusCompiler.exe"
      ${A_DIRECTORY} -output=${OUTPUT_DIR} -import=${A_DIRECTORY} -all
    SOURCES ${src}
  )
endfunction()
