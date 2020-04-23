mkdir build
cd build && cmake -G "Visual Studio 16 2019" -DCMAKE_TOOLCHAIN_FILE=vcpkg\scripts\buildsystems\vcpkg.cmake ..
cd ..
