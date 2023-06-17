function(apply_default_settings)
  cmake_parse_arguments(A "" "" "TARGETS" ${ARGN})
  foreach(arg TARGETS)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  foreach(target ${A_TARGETS})
    set_target_properties(${target} PROPERTIES
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
    if(MSVC)
      target_compile_features(${target} PRIVATE cxx_std_20)
      target_compile_options(${target} PRIVATE
        /wd4551 # disable non critical frida warning
        /wd5104 # disable non critical winsdk warning
        /wd5105 # TODO: investigate and fix
        /MP
        /utf-8	# Set Source and Executable character sets to UTF-8
        /Zi	# Debug Information Format

        /permissive-	# Standards conformance

        /Zc:alignedNew	# C++17 over-aligned allocation
        /Zc:__cplusplus	# Enable updated __cplusplus macro
        /Zc:externConstexpr	# Enable extern constexpr variables
        /Zc:preprocessor	# Enable preprocessor conformance mode
        #/Zc:throwingNew # Assume operator new throws on failure

        "$<$<CONFIG:DEBUG>:>"
        "$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob2>"
      )

      target_link_options(${target} PRIVATE
        #/WX	# Treat Linker Warnings as Errors

        "$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
        "$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
      )
    endif()
  endforeach()
endfunction()
