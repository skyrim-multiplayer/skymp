set(VCPKG_TARGET_ARCHITECTURE x64)

set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Keep in sync with skyrim-platform\tools\dev_service\index.js, requiredVcpkgDlls constant
# Note: at this moment, this list is empty. it was "spdlog" and "fmt" before, but now we use static linking for them.
if(${PORT} MATCHES "this_port_is_not_real|this_port_is_not_real")
  set(VCPKG_CRT_LINKAGE static) # VCPKG_CRT_LINKAGE should be the same for all ports
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

# The node-embedder-api takes a long time to build, generates an inconsistent number of libraries, and doesn't seem to integrate well with GitHub Actions' binary cache.
if(${PORT} MATCHES "node-embedder-api")
  set(VCPKG_BUILD_TYPE release)
endif()
