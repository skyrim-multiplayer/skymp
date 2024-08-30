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
vcpkg_cmake_config_fixup(CONFIG_PATH "lib/cmake/zf_log/")
