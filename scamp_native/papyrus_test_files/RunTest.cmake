message(STATUS SKYRIM_DIR=${SKYRIM_DIR})
message(STATUS SCRIPT_NAME=${SCRIPT_NAME})
message(STATUS OUTPUT_DIR=${OUTPUT_DIR})
message(STATUS SCRIPTS_DIR=${SCRIPTS_DIR})
message(STATUS VM_MAIN_PATH=${VM_MAIN_PATH})

execute_process(COMMAND "${SKYRIM_DIR}\\Papyrus Compiler\\PapyrusCompiler.exe"
  ${SCRIPT_NAME}.psc -output=${OUTPUT_DIR} -import=${SCRIPTS_DIR}
  RESULT_VARIABLE res
)

if (NOT ${res} STREQUAL "0")
  message(STATUS "Papyrus Compiler path is ${SKYRIM_DIR}\\Papyrus Compiler\\PapyrusCompiler.exe")
  message(FATAL_ERROR "PapyrusCompiler.exe failed with \n${res}")
endif()

execute_process(COMMAND ${VM_MAIN_PATH}
  ${OUTPUT_DIR}/${SCRIPT_NAME}.pex
  RESULT_VARIABLE res
)

if (NOT ${res} STREQUAL "0")
  message(FATAL_ERROR "${VM_MAIN_PATH} failed with \n${res}")
endif()
