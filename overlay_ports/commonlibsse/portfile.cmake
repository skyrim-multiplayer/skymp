vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Ryan-rsm-McKenzie/CommonLibSSE
    REF 30f7895da2945c602a8d160aa222d897ae223226
    SHA512 9577fc92877248b9edaf28903885805255243b3c8407373eaacf9c71f307cd030b7772b82df4917ae2cfe0e7548b8f4d17b2a8bccc45328e253dc6a0c3cd8f10
    HEAD_REF master
    PATCHES
      objectrefr-make_moverefr_public.patch
      variable-make_members_public.patch
      stackframe-uncomment_top_args.patch
      extradatalist-make_members_public.patch
      version-add_1.6.342_support.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
