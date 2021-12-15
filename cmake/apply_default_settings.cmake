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
			    /Zc:auto	# Deduce Variable Type
			    /Zc:char8_t
		    	/Zc:__cplusplus	# Enable updated __cplusplus macro
		    	/Zc:externC
	    		/Zc:externConstexpr	# Enable extern constexpr variables
	    		/Zc:forScope	# Force Conformance in for Loop Scope
	    		/Zc:implicitNoexcept	# Implicit Exception Specifiers
		    	/Zc:lambda
	    		/Zc:noexceptTypes	# C++17 noexcept rules
		    	#/Zc:preprocessor	# Enable preprocessor conformance mode
		    	/Zc:referenceBinding	# Enforce reference binding rules
	    		/Zc:rvalueCast	# Enforce type conversion rules
	    		/Zc:sizedDealloc	# Enable Global Sized Deallocation Functions
	    		/Zc:strictStrings	# Disable string literal type conversion
	    		/Zc:threadSafeInit	# Thread-safe Local Static Initialization
	    		/Zc:trigraphs	# Trigraphs Substitution
	    		/Zc:wchar_t	# wchar_t Is Native Type
      )
    endif()
  endforeach()
endfunction()
