set(VCPKG_TARGET_ARCHITECTURE x64)

set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Keep in sync with skyrim-platform\tools\dev_service\index.js, requiredVcpkgDlls constant
if(${PORT} MATCHES "spdlog|fmt")
  set(VCPKG_CRT_LINKAGE static) # VCPKG_CRT_LINKAGE should be the same for all ports
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()

# The node-embedder-api takes a long time to build, generates an inconsistent number of libraries, and doesn't seem to integrate well with GitHub Actions' binary cache.
# Disabling Debug for all ports since we don't support Debug anyway.
# Can be set node-embedder-api only in the future if needed.
set(VCPKG_BUILD_TYPE release)
