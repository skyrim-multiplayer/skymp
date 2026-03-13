vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO biaks/prometheus-cpp-lite
  REF e6e54d2cbc1d3650d9ecec62d75404db7c9b653b  # v2.0
  SHA512 0
  HEAD_REF main
)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/include/prometheus" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
