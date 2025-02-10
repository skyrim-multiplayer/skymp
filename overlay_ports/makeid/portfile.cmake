vcpkg_download_distfile(ARCHIVE
    URLS "file://${CMAKE_CURRENT_LIST_DIR}/MakeID.h"
    FILENAME "MakeID.h-${VERSION}"
    SHA512 0
)

file(INSTALL "${ARCHIVE}" DESTINATION "${CURRENT_PACKAGES_DIR}/include" RENAME "MakeID.h")

set(license_text 
"Public Domain

This file is released in the hopes that it will be useful. Use in whatever way you like, but no guarantees that it
actually works or fits any particular purpose. It has been unit-tested and benchmarked though, and seems to do
what it was designed to do, and seems pretty quick at it too."
)

file(WRITE "${CURRENT_PACKAGES_DIR}/share/makeid/copyright" "${license_text}")
