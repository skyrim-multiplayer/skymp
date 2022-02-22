vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Ryan-rsm-McKenzie/CommonLibSSE
    REF 4bf99d0d8fc917dc803e3dc7668caa948a12b59c
    SHA512 23ac8932f92da746f28930d7138b1f565f03dbf6e8a92284b70df1cd09f56bd8d7d924ee6ae84ad9cd84982dff4943d68186bc0347769f208c66650b62ebfbaa
    HEAD_REF master
    PATCHES
      patches/objectrefr-make_moverefr_public.patch
      patches/variable-make_members_public.patch
      patches/stackframe-uncomment_top_args.patch
      patches/extradatalist-make_members_public.patch
      patches/expand-alias.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
