set(CEF_URL "https://cef-builds.spotifycdn.com/cef_binary_105.3.25%2Bg0ca6a9e%2Bchromium-105.0.5195.54_windows64.tar.bz2")

vcpkg_download_distfile(ARCHIVE
    URLS ${CEF_URL}
    FILENAME "cef.tar.bz2"
    SHA512 3974c4f9ad2741b80bec79476ecc2af285633005f6982e77cb92cc5c8f91ca1b4f60959c8090aad28edbf61073a78e037d867c54cef1afe3e6de9db90c3184c4
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
