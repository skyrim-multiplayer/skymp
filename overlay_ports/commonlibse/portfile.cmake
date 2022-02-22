vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Ryan-rsm-McKenzie/CommonLibSSE
    REF 575f84a5c3badd70807fe917939214030967bced
    SHA512 286876ac975e50b13a8ba433a79c7061887d3a2c907f397211cabed9464d24d0c72eb6da8ce2bcb49b4ab53b4f184892d9b562be9368ca3b98fdef42a1825796
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
