function(link_vcpkg_dependencies)
  cmake_parse_arguments(A "" "" "TARGETS" ${ARGN})
  foreach(arg TARGETS)
    if ("${A_${arg}}" STREQUAL "")
      message(FATAL_ERROR "Missing ${arg} argument")
    endif()
  endforeach()

  foreach(target ${A_TARGETS})
    if(MSVC)
      find_package(unofficial-chakracore CONFIG REQUIRED)
      target_link_libraries(${target} PUBLIC unofficial::chakracore::chakracore)
    endif()

    find_path(JSON_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)
    get_filename_component(JSON_INCLUDE_DIR ${JSON_INCLUDE_DIR} DIRECTORY)
    target_include_directories(${target} PUBLIC ${JSON_INCLUDE_DIR})

    find_path(HTTPLIB_INCLUDE_DIR NAMES httplib.h PATH_SUFFIXES include)
    get_filename_component(HTTPLIB_INCLUDE_DIR ${HTTPLIB_INCLUDE_DIR} DIRECTORY)
    target_include_directories(${target} PUBLIC ${HTTPLIB_INCLUDE_DIR})

    find_package(ZLIB REQUIRED)
    target_link_libraries(${target} PUBLIC ZLIB::ZLIB)

    find_path(MAKEID_INCLUDE_DIR NAMES MakeID.h-1.0.2)
    target_include_directories(${target} PUBLIC ${MAKEID_INCLUDE_DIR})

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

      find_package(Boost MODULE REQUIRED)
      find_package(robin_hood REQUIRED)

      find_path(SIMPLEINI_INCLUDE_DIRS "ConvertUTF.c")
      target_include_directories(${target} PRIVATE ${SIMPLEINI_INCLUDE_DIRS})

      target_link_libraries(${target}	PRIVATE	Boost::headers CommonLibSSE::CommonLibSSE robin_hood::robin_hood)

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
