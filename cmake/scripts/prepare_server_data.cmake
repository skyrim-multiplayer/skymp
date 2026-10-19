include(${CMAKE_CURRENT_LIST_DIR}/../download_skyrim_data.cmake)

if(NOT DEFINED DEST_DIR)
  message(FATAL_ERROR "DEST_DIR is required")
endif()

file(MAKE_DIRECTORY "${DEST_DIR}")

# Optional local source (Skyrim/Data or UNIT_DATA_DIR) provided by the build.
set(HAVE_SOURCE_DATA_DIR OFF)
if(DEFINED SOURCE_DATA_DIR AND NOT "${SOURCE_DATA_DIR}" STREQUAL "" AND EXISTS "${SOURCE_DATA_DIR}")
  set(HAVE_SOURCE_DATA_DIR ON)
endif()

# Mandatory vanilla masters used by default loadOrder.
set(REQUIRED_FILES
  "Skyrim.esm"
  "Update.esm"
  "Dawnguard.esm"
  "HearthFires.esm"
  "Dragonborn.esm"
)

foreach(FILE_NAME ${REQUIRED_FILES})
  set(FILE_PATH "${DEST_DIR}/${FILE_NAME}")
  if(NOT EXISTS "${FILE_PATH}" AND HAVE_SOURCE_DATA_DIR)
    set(SRC_FILE_PATH "${SOURCE_DATA_DIR}/${FILE_NAME}")
    if(EXISTS "${SRC_FILE_PATH}")
      file(COPY "${SRC_FILE_PATH}" DESTINATION "${DEST_DIR}")
    endif()
  endif()
endforeach()

download_skyrim_data("${DEST_DIR}")

# Optional files often used in local setups. Download failures are non-fatal.
set(URL_BASE "https://gitlab.com/pospelov/se-data/-/raw/main")
set(OPTIONAL_FILES
  "_ResourcePack.esl"
  "ccBGSSSE001-Fish.esm"
  "ccBGSSSE025-AdvDSGS.esm"
  "ccBGSSSE037-Curios.esl"
  "ccQDRSSE001-SurvivalMode.esl"
)

foreach(FILE_NAME ${OPTIONAL_FILES})
  set(FILE_PATH "${DEST_DIR}/${FILE_NAME}")
  if(NOT EXISTS "${FILE_PATH}")
    if(HAVE_SOURCE_DATA_DIR)
      set(SRC_FILE_PATH "${SOURCE_DATA_DIR}/${FILE_NAME}")
      if(EXISTS "${SRC_FILE_PATH}")
        file(COPY "${SRC_FILE_PATH}" DESTINATION "${DEST_DIR}")
      endif()
    endif()
  endif()

  if(NOT EXISTS "${FILE_PATH}")
    file(DOWNLOAD "${URL_BASE}/${FILE_NAME}" "${FILE_PATH}"
      STATUS status
      LOG log
    )
    list(GET status 0 status_code)
    if(NOT status_code EQUAL 0)
      file(REMOVE "${FILE_PATH}")
      message(STATUS "Optional data file '${FILE_NAME}' is not available from ${URL_BASE}")
    endif()
  endif()
endforeach()

# Ensure scripts directory exists and copy any bundled local .pex files.
set(SCRIPTS_DEST_DIR "${DEST_DIR}/scripts")
file(MAKE_DIRECTORY "${SCRIPTS_DEST_DIR}")

if(DEFINED PEX_SOURCE_DIR AND EXISTS "${PEX_SOURCE_DIR}")
  file(GLOB PEX_FILES "${PEX_SOURCE_DIR}/*.pex")
  foreach(PEX_FILE ${PEX_FILES})
    file(COPY "${PEX_FILE}" DESTINATION "${SCRIPTS_DEST_DIR}")
  endforeach()
endif()
