vcpkg_download_distfile(ARCHIVE
    URLS "https://skse.silverlock.org/beta/skse64_2_00_19.7z"
    FILENAME "skse64_2_00_19.7z"
    SHA512 fb0c72605ea59030647979e4f69029ddfc0e7e63b06390d6595ace9da7e43383306feceada19fdf575137baeb5cc930c8c602df6b70a154ca2cd3020c20807af
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

file(GLOB skse64_include "${SOURCE_PATH}/skse64_2_00_19/src/skse64/skse64/*.h")
file(INSTALL ${skse64_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64")

file(GLOB skse64_common_include "${SOURCE_PATH}/skse64_2_00_19/src/skse64/skse64_common/*.h")
file(INSTALL ${skse64_common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/skse64_common")

file(GLOB common_include "${SOURCE_PATH}/skse64_2_00_19/src/common/*.h")
file(INSTALL ${common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/common")

file(GLOB xbyak_include "${SOURCE_PATH}/skse64_2_00_19/src/skse64/xbyak/*.h")
file(INSTALL ${xbyak_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/xbyak")

file(INSTALL ${SOURCE_PATH}/skse64_2_00_19/src/skse64/skse64_license.txt DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)