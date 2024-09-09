vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO wonder-mice/zf_log
    REF 5a3f2b46c3f62b97e2b33fbdb9d391abb64644f7
    SHA512 bd81805105e17e4330d030493227074f09f3ffd8b42048eecdade93b72335b2ddcb0707bca5b52ff34852743535b0c3ad071b84fbfb8fa9b77280222a654e69c
    HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME "zf-log" CONFIG_PATH "lib/cmake/zf_log/")

# zf_log-config.cmake
# zf_log-debug.cmake
# zf_log-release.cmake
# zf_log.cmake

# rename zf_log to zf-log for each
file(RENAME "${CURRENT_PACKAGES_DIR}/share/zf-log/zf_log-config.cmake" "${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log-config.cmake")
file(RENAME "${CURRENT_PACKAGES_DIR}/share/zf-log/zf_log-debug.cmake" "${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log-debug.cmake")
file(RENAME "${CURRENT_PACKAGES_DIR}/share/zf-log/zf_log-release.cmake" "${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log-release.cmake")
file(RENAME "${CURRENT_PACKAGES_DIR}/share/zf-log/zf_log.cmake" "${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log.cmake")

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

#message("${CURRENT_PACKAGES_DIR}")
#file(READ tmp "${CURRENT_PACKAGES_DIR}/share/zf_log/zf_log-config.cmake")
#message("zf_log-config.cmake: ${tmp}")

vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log-config.cmake" "zf_log" "zf-log")

# also replace zf_log-*.cmake with zf-log-*.cmake

vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/share/zf-log/zf-log.cmake" "zf_log-*.cmake" "zf-log-*.cmake")
