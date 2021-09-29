vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO lfrazer/CommonLibVR
    REF 0fabedac02c4d97b7d54199098561be8c0c666ef
    SHA512 f0c94fb1b90d70d8ec4c62acc08d85d87080501b0f17214eba9fa3560c96395bf7ed0c1ad9d8a98d9c94e129ce01ba0f2138f7ad2da419dcadd85085b474b4ce
    HEAD_REF master
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt DESTINATION ${SOURCE_PATH})

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
)

vcpkg_install_cmake()
vcpkg_copy_pdbs()

# Should we also move includes from CommonLibVR in include/commonlibsse ?
file(INSTALL "${SOURCE_PATH}/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include/commonlibvr")

file(INSTALL "${SOURCE_PATH}/LICENSE" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)
