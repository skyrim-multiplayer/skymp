vcpkg_download_distfile(ARCHIVE
    URLS "file://${CMAKE_CURRENT_LIST_DIR}/MakeID.h"
    FILENAME "MakeID.h-${VERSION}"
    SHA512 b4993d53cd2b7f967982fb3a2277070a58b7f80528cffd6588280dea2e919dce710174a9a56e46a1b978786831a14bb8a9b49f93b93aa09ddcf097deb2ea73fe
)

file(INSTALL "${ARCHIVE}" DESTINATION "${CURRENT_PACKAGES_DIR}/include" RENAME "MakeID.h")

set(license_text 
"Public Domain

This file is released in the hopes that it will be useful. Use in whatever way you like, but no guarantees that it
actually works or fits any particular purpose. It has been unit-tested and benchmarked though, and seems to do
what it was designed to do, and seems pretty quick at it too."
)

file(WRITE "${CURRENT_PACKAGES_DIR}/share/makeid/copyright" "${license_text}")
