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

      find_library(SKSE64_LIBRARY_DEBUG skse64)
      string(REPLACE "/debug/lib/" "/lib/" SKSE64_LIBRARY_RELEASE ${SKSE64_LIBRARY_DEBUG})
      find_path(SKSE64_INCLUDE_DIR skse64/PluginAPI.h)
      target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${SKSE64_LIBRARY_DEBUG},${SKSE64_LIBRARY_RELEASE}>")
      target_include_directories(${target} PUBLIC ${SKSE64_INCLUDE_DIR})

      find_library(SKSE64_COMMON_LIBRARY_DEBUG skse64_common)
      string(REPLACE "/debug/lib/" "/lib/" SKSE64_COMMON_LIBRARY_RELEASE ${SKSE64_COMMON_LIBRARY_DEBUG})
      find_path(SKSE64_COMMON_INCLUDE_DIR skse64/PluginAPI.h)
      target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${SKSE64_COMMON_LIBRARY_DEBUG},${SKSE64_COMMON_LIBRARY_RELEASE}>")
      target_include_directories(${target} PUBLIC ${SKSE64_COMMON_INCLUDE_DIR})

      find_library(COMMON_LIBRARY_DEBUG common)
      string(REPLACE "/debug/lib/" "/lib/" COMMON_LIBRARY_RELEASE ${COMMON_LIBRARY_DEBUG})
      find_path(COMMON_INCLUDE_DIR skse64/PluginAPI.h)
      target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${COMMON_LIBRARY_DEBUG},${COMMON_LIBRARY_RELEASE}>")
      target_include_directories(${target} PUBLIC ${COMMON_INCLUDE_DIR})

      find_library(COMMONLIBSSE_LIBRARY_DEBUG CommonLibSSE)
      string(REPLACE "/debug/lib/" "/lib/" COMMONLIBSSE_LIBRARY_RELEASE ${COMMONLIBSSE_LIBRARY_DEBUG})
      find_path(COMMONLIBSSE_INCLUDE_DIR SKSE/API.h)
      target_link_libraries(${target} PUBLIC "$<IF:$<CONFIG:Debug>,${COMMONLIBSSE_LIBRARY_DEBUG},${COMMONLIBSSE_LIBRARY_RELEASE}>")
      target_include_directories(${target} PUBLIC ${COMMONLIBSSE_INCLUDE_DIR})

      # CommonLibSSE requirement
      target_link_libraries(${target} PUBLIC Version)
      target_compile_options(${target} PUBLIC "/FI\"ForceInclude.h\"" "/FI\"SKSE/Logger.h\"")

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
