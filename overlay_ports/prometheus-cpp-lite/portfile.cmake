vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO biaks/prometheus-cpp-lite
  REF e6e54d2cbc1d3650d9ecec62d75404db7c9b653b  # v2.0
  SHA512 dcf42be028f5b740ce61b7523047235c385a4bb9fcc610733196a5a066004f9390b4df878f05515595f6e959eadea1a544408d86a420abd2c6ef85cb887881cb
  HEAD_REF main
  PATCHES fix-missing-charconv.patch
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/include/prometheus" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
