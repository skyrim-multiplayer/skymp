function(add_papyrus_library_ck)
    set(options)
    set(oneValueArgs NAME DIRECTORY COMPILER_EXECUTABLE_PATH OUTPUT_DIR)
    set(multiValueArgs)
    cmake_parse_arguments(A
      "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN}
    )

    file(GLOB src ${A_DIRECTORY}/*.psc)

    if(NOT A_OUTPUT_DIR)
      message(FATAL_ERROR "OUTPUT_DIR was not specified")
    endif()
    if(EXISTS "${A_COMPILER_EXECUTABLE_PATH}")
      # Copy src to out and for each element change extension from .psc to .pex
      set(out)
      foreach(file ${src})
        get_filename_component(file "${file}" NAME_WE)
        set(file "${A_OUTPUT_DIR}/${file}.pex")
        list(APPEND out ${file})
      endforeach()

      add_custom_command(
        OUTPUT ${out}
        COMMAND "${A_COMPILER_EXECUTABLE_PATH}" ${A_DIRECTORY} -flags=${CMAKE_SOURCE_DIR}/cmake/TESV_Papyrus_Flags.flg -output=${A_OUTPUT_DIR} -import=${A_DIRECTORY} -all
        DEPENDS ${src}
      )
      add_custom_target(${A_NAME} ALL
        DEPENDS ${out}
        SOURCES ${src}
      )
    else()
      # dummy target for post build events
      add_custom_target(${A_NAME} ALL
        COMMAND ${CMAKE_COMMAND} -E sleep 0
      )
      if(NOT A_COMPILER_EXECUTABLE_PATH MATCHES "OFF.*")
        message(STATUS "Skipping optional ${A_NAME} target (requires Creation Kit)")
      endif()
    endif()
endfunction()
