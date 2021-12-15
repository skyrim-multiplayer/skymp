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
      CXX_STANDARD 20
    )
    if(MSVC)
      set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE " /DEBUG /OPT:REF ")
      target_compile_options(
        ${target} PUBLIC
          /MP
          /utf-8	# Set Source and Executable character sets to UTF-8
          /Zi	# Debug Information Format

          /permissive-	# Standards conformance

          /Zc:alignedNew	# C++17 over-aligned allocation
		    	/Zc:__cplusplus	# Enable updated __cplusplus macro
	    		/Zc:externConstexpr	# Enable extern constexpr variables
		    	#/Zc:preprocessor	# Enable preprocessor conformance mode
          #/Zc:throwingNew # Assume operator new throws on failure

          "$<$<CONFIG:DEBUG>:>"
			    "$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob2>"
      )
    endif()
  endforeach()
endfunction()
