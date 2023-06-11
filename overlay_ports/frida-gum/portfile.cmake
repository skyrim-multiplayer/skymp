set(FRIDA_URL "https://github.com/frida/frida/releases/download/16.0.19/frida-gum-devkit-16.0.19-windows-x86_64.exe")
set(FRIDA_FILENAME "frida-gum-devkit-16.0.19-windows-x86_64.exe")
set(FRIDA_SHA512 9058c9010b966a07a875e4b7855dab60db6182b3c827c8d723d10b09034c50a418c72793f3e53419ab2e088c343eb2fd637cdb02bb8b64a0a29f68a2ee813343)

vcpkg_download_distfile(ARCHIVE
    URLS ${FRIDA_URL}
    FILENAME ${FRIDA_FILENAME}
    SHA512 ${FRIDA_SHA512}
)

vcpkg_extract_source_archive_ex(
    OUT_SOURCE_PATH SOURCE_PATH
    ARCHIVE ${ARCHIVE}
    REF 16.0.19
    NO_REMOVE_ONE_LEVEL
)

file(INSTALL ${SOURCE_PATH}/frida-gum.h DESTINATION ${CURRENT_PACKAGES_DIR}/include)
file(INSTALL ${SOURCE_PATH}/frida-gum.lib DESTINATION ${CURRENT_PACKAGES_DIR}/lib)
file(INSTALL ${SOURCE_PATH}/frida-gum.lib DESTINATION ${CURRENT_PACKAGES_DIR}/debug/lib)

file(DOWNLOAD https://raw.githubusercontent.com/frida/frida-gum/698b356fef0ecfc3ac2818f0b387be90e93deeda/COPYING ${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright)
