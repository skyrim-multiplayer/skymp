vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO powerof3/CommonLibSSE
    REF e3626d228d60b92a82410accd475bffdd7245653
    SHA512 349036dd83e7886b57e3b59f2810f0fc87d5ca9a2f8580597b6a20c8a046afb806cd7d394c57cff8c2106eaa46af44ee8a3cd0a74791d93a5e41072a09866386
    HEAD_REF dev
    PATCHES
      patches/01-objectrefr-make_moverefr_public.patch
      patches/02-variable-make_members_public.patch
      
      patches/03-stackframe-uncomment_top_args.patch
      
      patches/04-extradatalist-make_members_public.patch
      # patches/05-expand-alias.patch
      patches/06-fix-destructor.patch
)

vcpkg_configure_cmake(
  SOURCE_PATH ${SOURCE_PATH} 
  OPTIONS
    -DSKYRIM_SUPPORT_AE=ON
)

vcpkg_install_cmake()
vcpkg_fixup_cmake_targets(CONFIG_PATH "lib/cmake/commonlibsse")
vcpkg_copy_pdbs()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
