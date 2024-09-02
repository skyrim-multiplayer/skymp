vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Ryan-rsm-McKenzie/CommonLibSSE
    REF 20276e9c1f6104c07747d0eddf6d290485ff6e5f
    SHA512 be201095e0065cbac884f465a73c5331e1e1a97e5442b4768b73c57cb9b5bac3f47a4b81730963ec2239ba45c619cbcb1ef685b534c85fd67db9801edbc2c834
    HEAD_REF master
    PATCHES
      patches/objectrefr-make_moverefr_public.patch
      patches/variable-make_members_public.patch
      patches/stackframe-uncomment_top_args.patch
      patches/extradatalist-make_members_public.patch
      patches/update-fmt.patch
      patches/expand-alias-se.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
