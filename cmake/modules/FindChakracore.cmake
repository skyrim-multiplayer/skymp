if(TARGET ChakraCore::ChakraCore)
  return()
endif()

find_path(ChakraCore_INCLUDE_DIR NAMES ChakraCore.h)
find_library(ChakraCore_LIBRARY NAMES ChakraCore)
mark_as_advanced(ChakraCore_INCLUDE_DIR ChakraCore_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ChakraCore REQUIRED_VARS ChakraCore_LIBRARY ChakraCore_INCLUDE_DIR)

set(ChakraCore_INCLUDE_DIRS ${ChakraCore_INCLUDE_DIR})
set(ChakraCore_LIBRARIES ${ChakraCore_LIBRARY})

if(ChakraCore_FOUND)
  add_library(ChakraCore::ChakraCore UNKNOWN IMPORTED)

  set_target_properties(ChakraCore::ChakraCore PROPERTIES IMPORTED_LOCATION ${ChakraCore_LIBRARIES} INTERFACE_INCLUDE_DIRECTORIES ${ChakraCore_INCLUDE_DIRS})
endif()