function(link_vcpkg_dependencies)
  cmake_parse_arguments(A "" "" "TARGETS" ${ARGN})
  foreach(arg TARGETS)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  foreach(target ${A_TARGETS})
    find_path(ChakraCore_INCLUDE_DIR NAMES ChakraCore.h)
    find_library(ChakraCore_LIBRARY_Debug NAMES ChakraCore)
    string(REPLACE "/debug/lib/" "/lib/" ChakraCore_LIBRARY_Release "${ChakraCore_LIBRARY_Debug}")
    target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${ChakraCore_LIBRARY_Debug},${ChakraCore_LIBRARY_Release}>")
    target_include_directories(${target} PUBLIC ${ChakraCore_INCLUDE_DIR})

    find_path(JSON_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
    get_filename_component(JSON_INCLUDE_DIR ${JSON_INCLUDE_DIR} DIRECTORY)
    target_include_directories(${target} PUBLIC ${JSON_INCLUDE_DIR})

    find_path(HTTPLIB_INCLUDE_DIR NAMES httplib.h PATH_SUFFIXES include)
    get_filename_component(HTTPLIB_INCLUDE_DIR ${HTTPLIB_INCLUDE_DIR} DIRECTORY)
    target_include_directories(${target} PUBLIC ${HTTPLIB_INCLUDE_DIR})

    find_package(ZLIB REQUIRED)
    target_link_libraries(${target} PUBLIC ZLIB::ZLIB)

    if(MSVC AND "${target}" MATCHES "skyrim_platform")
      find_library(MHOOH_LIBRARY_DEBUG mhook)
      string(REPLACE "/debug/lib/" "/lib/" MHOOH_LIBRARY_RELEASE ${MHOOH_LIBRARY_DEBUG})
      find_path(MHOOH_INCLUDE_DIR NAMES mhook.h PATH_SUFFIXES mhook-lib)
      target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${MHOOH_LIBRARY_DEBUG},${MHOOH_LIBRARY_RELEASE}>")
      target_include_directories(${target} PUBLIC ${MHOOH_INCLUDE_DIR})

      if (SKYRIM_SE)
        find_package(commonlibse REQUIRED CONFIGS CommonLibSSEConfig.cmake)
      else()
        find_package(commonlibae REQUIRED CONFIGS CommonLibSSEConfig.cmake)
      endif()

      # ???
      find_package(robin_hood REQUIRED)

      find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
      target_include_directories(${target} PRIVATE ${SIMPLEINI_INCLUDE_DIRS})

      target_link_libraries(${target}	PRIVATE	CommonLibSSE::CommonLibSSE robin_hood::robin_hood)

      find_package(directxtk CONFIG REQUIRED)
      find_package(directxmath CONFIG REQUIRED)
      target_link_libraries(${target} PUBLIC Microsoft::DirectXTK)
    endif()

    find_package(spdlog CONFIG REQUIRED)
    target_link_libraries(${target} PUBLIC spdlog::spdlog)

    find_package(OpenSSL REQUIRED)
    target_link_libraries(${target} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
  endforeach()
endfunction()
