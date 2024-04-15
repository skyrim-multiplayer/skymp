vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CommonLibSSE
    REF 26015a042947ef3787eac754d559e9653c664a2c
    SHA512 14ad2555baa155b3240ce53ef0cd527b01173925de1d2bd28ce03d135acd1f33495eaf1e9d2d1dda94052394b3f7dc975357778b0df3a8ce71f3320d2e57128c
    HEAD_REF dev
    PATCHES
      patches/01-objectrefr-make_moverefr_public.patch
      patches/02-variable-make_members_public.patch
      
      patches/03-stackframe-uncomment_top_args.patch
      
      patches/04-extradatalist-make_members_public.patch
      # patches/05-expand-alias.patch
      patches/06-fix-destructor.patch
)

vcpkg_configure_cmake(SOURCE_PATH ${SOURCE_PATH})

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
