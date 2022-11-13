vcpkg_download_distfile(ARCHIVE
    URLS "https://skse.silverlock.org/beta/skse64_2_02_03.7z"
    FILENAME "skse64_2_02_03.7z"
    SHA512 b57f33bf673c70b98c33d686c1bb28ce985719ab83cf4968bd0d4f9ee594126108fae1b825febab0dc3419cc19e3c72e1729d66d47367c80f406ca5e8aef9485
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    NO_REMOVE_ONE_LEVEL
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)
vcpkg_build_cmake()
vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(GLOB skse64_include "${SOURCE_PATH}/skse64_2_02_03/src/skse64/skse64/*.h")
file(INSTALL ${skse64_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64")

file(GLOB skse64_common_include "${SOURCE_PATH}/skse64_2_02_03/src/skse64/skse64_common/*.h")
file(INSTALL ${skse64_common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64_common")

file(GLOB common_include "${SOURCE_PATH}/skse64_2_02_03/src/common/*.h")
file(INSTALL ${common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/common")

file(GLOB xbyak_include "${SOURCE_PATH}/skse64_2_02_03/src/skse64/xbyak/*.h")
file(INSTALL ${xbyak_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/xbyak")

file(INSTALL ${SOURCE_PATH}/skse64_2_02_03/src/skse64/skse64_license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
