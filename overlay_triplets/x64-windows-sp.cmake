set(VCPKG_TARGET_ARCHITECTURE x64)

set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

# Keep in sync with skyrim-platform\tools\dev_service\index.js, requiredVcpkgDlls constant
if(${PORT} MATCHES "spdlog|fmt")
  set(VCPKG_CRT_LINKAGE static) # VCPKG_CRT_LINKAGE should be the same for all ports
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
