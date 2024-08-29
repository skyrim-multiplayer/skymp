vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mongodb/mongo-cxx-driver
    REF "aa0b94c2c33b1df06b22c777a78149f4e7c70b2b"
    SHA512 6980c50245c2bcdce4d4b97497dea31dc1a85b39ff0e6a2a971bd0033d6adf7af829e994f82da84ef74aadf1f1b3f6fe1fd404d7c4d1b38c950226f5cdb81e8e
    HEAD_REF master
    PATCHES
        fix-dependencies.patch
)
file(WRITE "${SOURCE_PATH}/build/VERSION_CURRENT" "${VERSION}")

# This port offered C++17 ABI alternative via features.
# This was reduced to boost only.
vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        boost   BSONCXX_POLY_USE_BOOST
    INVERTED_FEATURES
        boost   BSONCXX_POLY_USE_STD
        boost   CMAKE_DISABLE_FIND_PACKAGE_Boost
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        ${FEATURE_OPTIONS}
        "-DCMAKE_PROJECT_MONGO_CXX_DRIVER_INCLUDE=${CMAKE_CURRENT_LIST_DIR}/cmake-project-include.cmake"
        -DBSONCXX_HEADER_INSTALL_DIR=include
        -DENABLE_TESTS=OFF
        -DENABLE_UNINSTALL=OFF
        -DMONGOCXX_HEADER_INSTALL_DIR=include
        -DNEED_DOWNLOAD_C_DRIVER=OFF
    MAYBE_UNUSED_VARIABLES
        CMAKE_DISABLE_FIND_PACKAGE_Boost
        BSONCXX_HEADER_INSTALL_DIR
        MONGOCXX_HEADER_INSTALL_DIR
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

vcpkg_cmake_config_fixup(PACKAGE_NAME "bsoncxx" CONFIG_PATH "lib/cmake/bsoncxx-${VERSION}" DO_NOT_DELETE_PARENT_CONFIG_PATH)
vcpkg_cmake_config_fixup(PACKAGE_NAME "mongocxx" CONFIG_PATH "lib/cmake/mongocxx-${VERSION}")

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

function(auto_clean dir)
    file(GLOB entries "${dir}/*")
    file(GLOB files LIST_DIRECTORIES false "${dir}/*")
    foreach(entry IN LISTS entries)
        if(entry IN_LIST files)
            continue()
        endif()
        file(GLOB_RECURSE children "${entry}/*")
        if(children)
            auto_clean("${entry}")
        else()
            file(REMOVE_RECURSE "${entry}")
        endif()
    endforeach()
endfunction()
auto_clean("${CURRENT_PACKAGES_DIR}/include")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
