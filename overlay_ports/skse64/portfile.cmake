vcpkg_download_distfile(ARCHIVE
    URLS "https://skse.silverlock.org/beta/skse64_2_01_03.7z"
    FILENAME "skse64_2_01_03.7z"
    SHA512 844947f2990c4d10d2b035f7a3d80dcc462a8dac61fa84e3e07262d510d799bf0910e424b61cbb4c9e34033856203a9200562333b0fc4a1f9dd5ca3136330e59
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

file(GLOB skse64_include "${SOURCE_PATH}/skse64_2_01_03/src/skse64/skse64/*.h")
file(INSTALL ${skse64_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64")

#file(GLOB skse64_common_include "${SOURCE_PATH}/skse64_2_01_03/src/skse64/skse64_common/*.h")
#file(INSTALL ${skse64_common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64_common")

#file(GLOB common_include "${SOURCE_PATH}/skse64_2_01_03/src/common/*.h")
#file(INSTALL ${common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/common")

#file(GLOB xbyak_include "${SOURCE_PATH}/skse64_2_01_03/src/skse64/xbyak/*.h")
#file(INSTALL ${xbyak_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/xbyak")

file(INSTALL ${SOURCE_PATH}/skse64_2_01_03/src/skse64/skse64_license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
