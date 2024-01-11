set(VCPKG_TARGET_ARCHITECTURE x64)

set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

if(${PORT} MATCHES "chakracore")
  set(VCPKG_CRT_LINKAGE static) # VCPKG_CRT_LINKAGE should be the same for all ports
  set(VCPKG_LIBRARY_LINKAGE dynamic)
endif()
