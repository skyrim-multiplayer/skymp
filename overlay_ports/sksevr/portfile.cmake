vcpkg_download_distfile(ARCHIVE
    URLS "https://skse.silverlock.org/beta/sksevr_2_00_12.7z"
    FILENAME "sksevr_2_00_12.7z"
    SHA512 9042a86d19a28c4a8cc3803e8153ecd7c27f2c311ab92863c28a2e550d6653728dc8df24fa4dcb3797f118bffbd38f199e3fb169fcfd4cd4a2820c7ce4d49a3d
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

file(GLOB sksevr_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/skse64/*.h")
file(INSTALL ${sksevr_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/sksevr")

file(GLOB sksevr_common_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/skse64_common/*.h")
file(INSTALL ${sksevr_common_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/sksevr_common")

file(GLOB commonvr_include "${SOURCE_PATH}/sksevr_2_00_12/src/common/*.h")
file(INSTALL ${commonvr_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/commonvr")

# Not sure if I also need xbyak from sksevr, they look the same.
file(GLOB xbyak_include "${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/xbyak/*.h")
file(INSTALL ${xbyak_include} DESTINATION "${CURRENT_PACKAGES_DIR}/include/xbyakvr")

# Unlike skse64_2_00_19, there are no license file in sksevr_2_00_12. I used xbyak's instead as I got an error without a file.
file(INSTALL ${SOURCE_PATH}/sksevr_2_00_12/src/sksevr/xbyak/COPYRIGHT DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
