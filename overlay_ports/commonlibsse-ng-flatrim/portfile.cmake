vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO CharmedBaryon/CommonLibSSE
    REF b93280e832f263dbef44e44cbe2936622a02f91a
    SHA512  c98a0dde8fab45d0b5ffc8241bc6437fc1b2855e5577e9b66fe4e237c35c7111c3288f0ef7a637b2a399dd38938221619331e3b49504b76ac0e1a0a2034715a6
    HEAD_REF main
    PATCHES
      patches/01-objectrefr-make_moverefr_public.patch
      patches/02-variable-make_members_public.patch
      patches/03-stackframe-uncomment_top_args.patch
      patches/04-extradatalist-make_members_public.patch
)

vcpkg_configure_cmake(
        SOURCE_PATH "${SOURCE_PATH}"
        PREFER_NINJA
        OPTIONS -DENABLE_SKYRIM_VR=off -DBUILD_TESTS=off -DSKSE_SUPPORT_XBYAK=on
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(PACKAGE_NAME CommonLibSSE CONFIG_PATH lib/cmake)
vcpkg_copy_pdbs()

file(GLOB CMAKE_CONFIGS "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE/*.cmake")
file(INSTALL ${CMAKE_CONFIGS} DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")
file(INSTALL "${SOURCE_PATH}/cmake/CommonLibSSE.cmake" DESTINATION "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/share/CommonLibSSE/CommonLibSSE")

file(
        INSTALL "${SOURCE_PATH}/LICENSE"
        DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
        RENAME copyright)
