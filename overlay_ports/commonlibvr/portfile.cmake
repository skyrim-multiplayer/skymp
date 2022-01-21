vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO alandtse/CommonLibVR
    REF 150e82b78131fe08ce9bf57af41802550fdea421
    SHA512 0ab74ee08ae18b099e8e7bae2488145b5cf763e53a429a2d97661f82a77c37b6482c52c7febd5e395319cd59d7fcb08420c480a965fc05e7998a1ef937fee33f
    HEAD_REF vr
    PATCHES
      default-vr-on.patch
      variable-make_members_public.patch
      stackframe-uncomment-top-args.patch
      objectrefr-make_moverefr_public.patch
      extradatalist-make_members_public.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibvr")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
