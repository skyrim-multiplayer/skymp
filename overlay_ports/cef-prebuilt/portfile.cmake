set(CEF_URL "https://cef-builds.spotifycdn.com/cef_binary_107.1.9+g1f0a21a+chromium-107.0.5304.110_windows64.tar.bz2")

vcpkg_download_distfile(ARCHIVE
    URLS ${CEF_URL}
    FILENAME "cef.tar.bz2"
    SHA512 81e4ce641cbf44f35bc09ff471d1893b9ce037ce27f399af8898be429b745a8e0f5fa8cfa203965f55b74d335dec94f7be4b9a73b0f0fbe71b5fbab0f8116e49
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

file(INSTALL "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}")
file(INSTALL "${SOURCE_PATH}/Debug" "${SOURCE_PATH}/Release" "${SOURCE_PATH}/Resources" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
