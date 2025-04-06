vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO CharmedBaryon/CommonLibSSE
    REF c4ab853d095e81e3390b282d7ba01ab2f24ebf25
    SHA512  fd615c16f8f2c637cad5ed9d139c776d21314664f4084a62231645114d03ee74e720c1ecf09b4e5daa5d56d418374ad6d587806788d95af8ac08ce3de930015b
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
