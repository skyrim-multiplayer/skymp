include(CMakeFindDependencyMacro)

if(NOT TARGET unofficial::ga-sdk-cpp::ga-sdk-cpp)
  add_library(unofficial::ga-sdk-cpp::ga-sdk-cpp UNKNOWN IMPORTED)

  find_path(ga-sdk-cpp_INCLUDE_DIR NAMES GameAnalytics.h)

  set_target_properties(unofficial::ga-sdk-cpp::ga-sdk-cpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${ga-sdk-cpp_INCLUDE_DIR}"
  )

  find_library(ga-sdk-cpp_LIBRARY_RELEASE NAMES ga-sdk-cpp PATHS "${CMAKE_CURRENT_LIST_DIR}/../../lib" NO_DEFAULT_PATH REQUIRED)
  find_library(ga-sdk-cpp_LIBRARY_DEBUG NAMES ga-sdk-cpp PATHS "${CMAKE_CURRENT_LIST_DIR}/../../debug/lib" NO_DEFAULT_PATH REQUIRED)

  set_target_properties(unofficial::ga-sdk-cpp::ga-sdk-cpp PROPERTIES
    IMPORTED_LOCATION_DEBUG "${ga-sdk-cpp_LIBRARY_DEBUG}"
    IMPORTED_LOCATION_RELEASE "${ga-sdk-cpp_LIBRARY_RELEASE}"
    IMPORTED_CONFIGURATIONS "Release;Debug"
  )

  find_dependency(CURL REQUIRED)
  find_dependency(OpenSSL REQUIRED)
  find_dependency(RapidJSON REQUIRED)
  find_dependency(SQLite3 REQUIRED)
  find_dependency(miniz REQUIRED)
  find_dependency(zf-log REQUIRED)

  target_link_libraries(unofficial::ga-sdk-cpp::ga-sdk-cpp INTERFACE 
    CURL::libcurl
    OpenSSL::SSL
    OpenSSL::Crypto
    rapidjson
    SQLite::SQLite3
    miniz::miniz
    zf_log
  )
endif()
