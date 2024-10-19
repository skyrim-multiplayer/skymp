# TODO: fix Linux build, should copy .so, not .a

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO nodejs/node
  REF "v${VERSION}"
  SHA512 cb88b3576ac810ceace7d5824d5a6e1c4181a9327f1420d6eb1546a03a22f5f80bdddcbef6bc478ce4cb62dcd724b7a04eda707845a4003b96a5d6aed5463b37
  HEAD_REF main
)

# Fixes arm64-windows host building x64-windows target
vcpkg_replace_string("${SOURCE_PATH}/configure.py" "'ARM64'  : 'arm64'" "'ARM64'  : 'x64'")

vcpkg_find_acquire_program(PYTHON3)
get_filename_component(PYTHON3_EXE_PATH ${PYTHON3} DIRECTORY)
vcpkg_add_to_path(PREPEND "${PYTHON3_EXE_PATH}")

if(VCPKG_TARGET_IS_WINDOWS)
  # dll/static is an option. 
  # dll is recommended for embedding by nodejs authors 
  # https://github.com/nodejs/node/blob/70fcb87af4c41be4f480b213d8b3edfc49629c9f/configure.py#L865
  set(nodejs_options openssl-no-asm dll ${VCPKG_TARGET_ARCHITECTURE})

  if(NOT "${VCPKG_BUILD_TYPE}" STREQUAL "release")
    message(STATUS "Building nodejs Debug")

    execute_process(
      COMMAND "${SOURCE_PATH}/vcbuild.bat" debug ${nodejs_options}
      WORKING_DIRECTORY "${SOURCE_PATH}"

      OUTPUT_VARIABLE NODE_BUILD_SH_OUT
      ERROR_VARIABLE NODE_BUILD_SH_ERR
      RESULT_VARIABLE NODE_BUILD_SH_RES
      ECHO_OUTPUT_VARIABLE
      ECHO_ERROR_VARIABLE
    )
    if(NOT NODE_BUILD_SH_RES EQUAL 0)
      message(FATAL_ERROR "Failed to build nodejs Debug (code ${NODE_BUILD_SH_RES})")
    endif()

    set(dll_path_debug "${SOURCE_PATH}/Debug/libnode.dll")
    set(lib_path_debug "${SOURCE_PATH}/Debug/libnode.lib")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/bin/node-embedder-api")
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/lib/node-embedder-api")
    file(COPY "${dll_path_debug}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin/node-embedder-api")
    file(COPY "${lib_path_debug}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib/node-embedder-api")
  endif()

  message(STATUS "Building nodejs Release")

  execute_process(
    COMMAND "${SOURCE_PATH}/vcbuild.bat" release ${nodejs_options}
    WORKING_DIRECTORY "${SOURCE_PATH}"

    OUTPUT_VARIABLE NODE_BUILD_SH_OUT
    ERROR_VARIABLE NODE_BUILD_SH_ERR
    RESULT_VARIABLE NODE_BUILD_SH_RES
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
  )

  if(NOT NODE_BUILD_SH_RES EQUAL 0)
    message(FATAL_ERROR "Failed to build nodejs Release (code ${NODE_BUILD_SH_RES})")
  endif()

  set(dll_path_release "${SOURCE_PATH}/Release/libnode.dll")
  set(lib_path_release "${SOURCE_PATH}/Release/libnode.lib")
  file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/bin/node-embedder-api")
  file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/node-embedder-api")
  file(COPY "${dll_path_release}" DESTINATION "${CURRENT_PACKAGES_DIR}/bin/node-embedder-api")
  file(COPY "${lib_path_release}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib/node-embedder-api")
else()
  find_program(MAKE make REQUIRED)

  if(NOT "${VCPKG_BUILD_TYPE}" STREQUAL "release")
    message(STATUS "Configuring nodejs Debug")

    execute_process(
      COMMAND "${SOURCE_PATH}/configure" "--debug"
      WORKING_DIRECTORY "${SOURCE_PATH}"

      OUTPUT_VARIABLE NODE_BUILD_SH_OUT
      ERROR_VARIABLE NODE_BUILD_SH_ERR
      RESULT_VARIABLE NODE_BUILD_SH_RES
      ECHO_OUTPUT_VARIABLE
      ECHO_ERROR_VARIABLE
    )

    if(NOT NODE_BUILD_SH_RES EQUAL 0)
      message(FATAL_ERROR "Failed to configure nodejs debug (code ${NODE_BUILD_SH_RES})")
    endif()

    message(STATUS "Building nodejs Debug")
    
    vcpkg_execute_build_process(
      COMMAND "${MAKE}" "-j${VCPKG_CONCURRENCY}" ${MAKE_OPTIONS}
      NO_PARALLEL_COMMAND "${MAKE}" "-j1" ${MAKE_OPTIONS}
      WORKING_DIRECTORY "${SOURCE_PATH}"
      LOGNAME "build-${TARGET_TRIPLET}-dbg"
    )

    file(GLOB libs "${SOURCE_PATH}/Debug/*.a")
    foreach(path ${libs})
      file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/debug/lib/node-embedder-api")
      file(COPY "${path}" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib/node-embedder-api")
    endforeach()
  endif()

  message(STATUS "Configuring nodejs Release")

  execute_process(
    COMMAND "${SOURCE_PATH}/configure"
    WORKING_DIRECTORY "${SOURCE_PATH}"

    OUTPUT_VARIABLE NODE_BUILD_SH_OUT
    ERROR_VARIABLE NODE_BUILD_SH_ERR
    RESULT_VARIABLE NODE_BUILD_SH_RES
    ECHO_OUTPUT_VARIABLE
    ECHO_ERROR_VARIABLE
  )

  if(NOT NODE_BUILD_SH_RES EQUAL 0)
    message(FATAL_ERROR "Failed to configure nodejs release (code ${NODE_BUILD_SH_RES})")
  endif()

  message(STATUS "Building nodejs Release")
  
  vcpkg_execute_build_process(
    COMMAND "${MAKE}" "-j${VCPKG_CONCURRENCY}" ${MAKE_OPTIONS}
    NO_PARALLEL_COMMAND "${MAKE}" "-j1" ${MAKE_OPTIONS}
    WORKING_DIRECTORY "${SOURCE_PATH}"
    LOGNAME "build-${TARGET_TRIPLET}-rel"
  )
  
  file(GLOB libs "${SOURCE_PATH}/Release/*.a")
  foreach(path ${libs})
    file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/lib/node-embedder-api")
    file(COPY "${path}" DESTINATION "${CURRENT_PACKAGES_DIR}/lib/node-embedder-api")
  endforeach()

endif()

file(MAKE_DIRECTORY "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")

# main header
file(COPY "${SOURCE_PATH}/src/node.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")

# node.h requirements
file(COPY "${SOURCE_PATH}/src/node_api.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")
file(COPY "${SOURCE_PATH}/src/node_version.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")

file(GLOB found_headers "${SOURCE_PATH}/deps/v8/include/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/deps/v8/include/cppgc/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/cppgc")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/deps/v8/include/cppgc/internal/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/cppgc/internal")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/deps/v8/include/libplatform/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/libplatform")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/deps/uv/include/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/deps/uv/include/uv/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/uv")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/crypto/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/crypto")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/dataqueue/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/dataqueue")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/inspector/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/inspector")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/large_pages/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/large_pages")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/permission/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/permission")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/quic/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/quic")
endforeach()

file(GLOB found_headers "${SOURCE_PATH}/src/tracing/*.h")
foreach(found_header ${found_headers})
  file(COPY "${found_header}" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api/tracing")
endforeach()

# node_api.h requirements
file(COPY "${SOURCE_PATH}/src/js_native_api.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")
file(COPY "${SOURCE_PATH}/src/node_api_types.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")

# js_native_api.h requirements
file(COPY "${SOURCE_PATH}/src/js_native_api_types.h" DESTINATION "${CURRENT_PACKAGES_DIR}/include/node-embedder-api")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
