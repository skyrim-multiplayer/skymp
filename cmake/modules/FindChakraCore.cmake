if(TARGET ChakraCore::ChakraCore)
  return()
endif()

find_path(ChakraCore_INCLUDE_DIR NAMES ChakraCore.h)
find_library(ChakraCore_LIBRARY_Debug NAMES ChakraCore)
string(REPLACE "/debug/lib/" "/lib/" ChakraCore_LIBRARY_Release "${ChakraCore_LIBRARY_Debug}")
mark_as_advanced(ChakraCore_INCLUDE_DIR ChakraCore_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ChakraCore REQUIRED_VARS ChakraCore_LIBRARY ChakraCore_INCLUDE_DIR)

if(ChakraCore_FOUND)
  add_library(ChakraCore::ChakraCore UNKNOWN IMPORTED)

  set_target_properties(ChakraCore::ChakraCore PROPERTIES 
    IMPORTED_LOCATION_RELEASE ${ChakraCore_LIBRARY_Release}
    IMPORTED_LOCATION_DEBUG ${ChakraCore_LIBRARY_Debug}
    INTERFACE_INCLUDE_DIRECTORIES ${ChakraCore_INCLUDE_DIR}
  )

  unset(ChakraCore_LIBRARY_Release)
  unset(ChakraCore_LIBRARY_Debug)
endif()
