vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO nic11/antigo
  REF df74670a2d1314bf1f9e38eb46f1c4fae4a88cef
  SHA512 ea565de2e4c184be1580ab5928a24964d605d259673baa27c98b879b156effdf697f6c73b5ab276171aa72b575d234364414c1f310649a05e7fa169fb0ccd58d
  HEAD_REF master
  PATCHES
    patches/tmp.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_cmake_install()
vcpkg_cmake_config_fixup()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
