vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO Pospelove/GA-SDK-CPP
    REF 93b65d9f122794673c9d65382671d18dd7a599f3
    SHA512 0ba69a33886eed8e4be15b09d3f21ef4428c0e8375f4af1e0cf892a68e3c9af2022215c406a8426640ae3e23902384314d4e42feadadab2d7769ee9ce37a5fdd
    HEAD_REF patch-1
    PATCHES void.patch
)

# copy cmakrlists.txt
file(COPY "${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt" DESTINATION "${SOURCE_PATH}")

# add missing includes
file(READ "${SOURCE_PATH}/source/gameanalytics/GADevice.cpp" GADevice_cpp)
file(WRITE "${SOURCE_PATH}/source/gameanalytics/GADevice.cpp" "#include <tchar.h>\n#include <iostream>\n${GADevice_cpp}")

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
)
vcpkg_cmake_install()
