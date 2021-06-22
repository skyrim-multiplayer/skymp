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
      CXX_EXTENSIONS OFF
      CXX_STANDARD 17
    )
    if (MSVC)
      set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " /DEBUG /OPT:REF ")
      target_compile_options(${target} PUBLIC "/MP")
      target_compile_options(${target} PUBLIC "$<$<CONFIG:Release>:/Zi>")
      #target_compile_options(${target} PUBLIC "/permissive-")
    endif()
  endforeach()

endfunction()
