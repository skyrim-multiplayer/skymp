vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO nic11/antigo
  REF da15297955935da32e455fc1ca2a12faf564caae
  SHA512 1d94bc13e05a6572fce3295d3e0c8ae8b16d2ba2b08e5d13b69a0816dba61d28ee16c3fec92ea65b375ce1753063c0f22379a075770702d938f8234d921570c3
  HEAD_REF master
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
