# set(VCPKG_PATH_REL "${CMAKE_CURRENT_LIST_DIR}/../../vcpkg")
# get_filename_component(VCPKG_PATH_ABS "${VCPKG_PATH_REL}" ABSOLUTE)

vcpkg_from_github(
  OUT_SOURCE_PATH SOURCE_PATH
  REPO kkEngine/NirnLabUIPlatform
  REF 2f04e3f2335644105b50f7e806767cd5afc67c91
  SHA512 1984d9e1a2a4a376969faa373e1154d297f83ed4ef5c85ae9995a59c6766884928981c541a01903f7648502482e020371a188c7b2aa5a40837c48000fbd8ff05
  HEAD_REF main
  PATCHES tmp.patch
)

# set(ENV{VCPKG_ROOT} ${VCPKG_PATH_ABS})

# vcpkg_configure_cmake(
#   SOURCE_PATH "${SOURCE_PATH}"
#   # OPTIONS "-DVCPKG_ROOT=${VCPKG_PATH_ABS}"
#   # OPTIONS
#   #   --debug-find
#   #   --trace
# )

# vcpkg_cmake_install()
# vcpkg_cmake_config_fixup()
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

file(COPY "${SOURCE_PATH}/src/UIPlatform/NirnLabUIPlatformAPI" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
