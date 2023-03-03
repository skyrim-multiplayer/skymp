vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Ryan-rsm-McKenzie/CommonLibSSE
    REF 0e9d380b90950eb3ece1e5b95e3b6a379ee03f8e
    SHA512 80afa2c9444f4bbb873e8de4a4af8b38b19af2ac3b60d0d68ca25d02b1b3f4bdac83ebe30ced2385c8fc90bb0e0dcf64f9fc05f896951a0dd60a799bc8d53a35
    HEAD_REF master
    PATCHES
      patches/01-objectrefr-make_moverefr_public.patch
      patches/02-variable-make_members_public.patch
      patches/03-stackframe-uncomment_top_args.patch
      patches/04-extradatalist-make_members_public.patch
      patches/05-expand-alias.patch
      patches/06-fix-destructor.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
