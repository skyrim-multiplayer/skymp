<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://skymp.io">
    <img src="skymp.jpg" alt="Logo" width="200" height="200">
  </a>
  <h3 align="center">SkyMP</h3>

  <p align="center">
    A general-purpose multiplayer mod for Skyrim SE. 
    <br>
    <a href="https://github.com/skyrim-multiplayer/skymp/tree/main/docs">Explore Docs</a>
    ·
    <a href="https://github.com/skyrim-multiplayer/skymp/issues">Report Bug</a>
    ·
    <a href="https://github.com/skyrim-multiplayer/skymp/issues">Request Feature</a> 
    ·
    <a href="https://discord.gg/k39uQ9Yudt">Join Discord</a>
  </p>
</p>

## Building From Source

You can find instructions on setting up the project locally below. To get a local copy up and running follow these simple example steps. You need ~4 GB RAM, ~22 GB on your hard drive and some free time.

## Prerequisites

### Common

These tools required regardless of your system:

* 64-bit [NodeJS](https://nodejs.org/en/download/) 12.x/14.x/16.x
* [Yarn](https://yarnpkg.com/getting-started/install): `npm install --global yarn`
* [CMake 3.18.2](https://cmake.org/download/) or higher

### Windows

Before your start make sure that your system meets the conditions:

* Windows 7 or higher *([Windows 10](https://www.microsoft.com/en-us/software-download/windows10) is recommended)*
* [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* .NET Framework SDK at 4.6.0 or higher *(Visual Studio Installer -> .NET desktop development)*

### Linux

You can build and run server and unit-tests on Linux.
As Skyrim has no native Linux version, client can only be built using MSVC.
Client can be run with Proton (though it can be tricky to get Skyrim itself to work properly).

* Ubuntu *(Some other distributions may also work, but we know that Alpine doesn't)*
* Clang 12 *(GCC is not supported)*: `sudo apt install clang-12`
* Python 2 (not 3.x! It is needed to build ChakraCore. Don't worry, it won't conflict with Python 3):
  `sudo apt install python2`
* Make sure that your NodeJS and CMake are fresh enough:
  * You can use [`nvm`](https://github.com/nvm-sh/nvm) or [Nodesource's apt repositories](https://github.com/nodesource/distributions) to install fresh Node
  * The simpliest way to install fresh CMake is to download a `.tar.gz` from [CMake download page](https://cmake.org/download/),
    unpack it to your home directory and add it to path:
    ```sh
    echo 'export PATH="$HOME/apps/cmake-3.22.0-.../bin:$PATH"' >> ~/.bashrc
    ```

## Configuring and Building

### Common

1. Clone the repo, including submodules
   ```sh
   git clone https://github.com/skyrim-multiplayer/skymp.git
   cd skymp
   git submodule init
   git submodule update
   ```

2. Do OS-specific steps (see below)

### Windows

1. Make a build directory (used for project files, cache, artifacts, etc)
   ```sh
   mkdir build
   ```
2. Generate project files with CMake (replace path with your actual Skyrim SE folder)
   ```sh
   cd build
   cmake .. -DSKYRIM_DIR="C:/Program Files (x86)/Steam/steamapps/common/Skyrim Special Edition"
   ```
   For users who don't have Skyrim SE installed:
   ```sh
   cd build
   cmake ..
   ```
   * Some tests would be skipped
   * The server would require manual installation of Skyrim.esm and other master files
   * Papyrus scripts that require Bethesda's compiler would not be compiled, prebuilts would be used

3. Build with CMake:
   ```sh
   cmake --build . --config Debug
   ```
   On Windows you also can open `build/skymp.sln` with Visual Studio, then `Build -> Build Solution`.

   All build artifacts would be placed into `build/dist`.

#### How to build only SkyrimPlatform

-  For those users who want to build SP but not SkyMP there is `ONLY_SP` option:

   ```sh
   cmake .. -DONLY_SP=ON
   ```

### Linux

On Linux, there might be some tricky dependency issues. To work around them,
we recommend you to use a wrapper script `build.sh`. It will create a temporary
directory and add some aliases to `PATH`.

1. Generate project files with CMake wrapper (replace path with your actual Skyrim SE folder)
   ```sh
   ./build.sh --configure -DCMAKE_BUILD_TYPE=Debug \
      -DSKYRIM_DIR="$HOME/.steam/debian-installation/steamapps/common/Skyrim Special Edition"
   ```
   For users who don't have Skyrim SE installed:
   ```sh
   ./build.sh --configure -DCMAKE_BUILD_TYPE=Debug
   ```
   If you're building for a production machine, change build type to Release:
   ```sh
   ./build.sh --configure -DCMAKE_BUILD_TYPE=Release
   ```

2. Build with CMake wrapper:
   ```sh
   cd build
   ../build.sh --build
   ```
   Additional arguments after `--build` will be passed to CMake. E.g. you can specify the build target:
   ```sh
   cd build
   ../build.sh --build --target=unit  # only build unit-tests and their dependencies
   # Will run cmake --build . --target=unit
   ```

### Optional steps after build

1. Run tests:
   ```sh
   ctest -C Debug --verbose
   ```
   Some tests ([ESPMTest](https://github.com/skyrim-multiplayer/skymp/blob/main/skymp5-server/cpp/unit/EspmTest.cpp)) require Skyrim SE data files and will be skipped if you didn't specify `-DSKYRIM_DIR`.

   In order to avoid potential errors, make sure:
   1. You have installed it using Steam and it's up to date (currently last update was on [Nov 20, 2019](https://steamdb.info/depot/489832/history/?changeid=M:8702665189575304780)). See SteamDB for [hashes](https://steamdb.info/depot/489832/?show_hashes) and [update history](https://steamdb.info/depot/489832/history/).
   2. You did not modify `Skyrim.esm`, `Update.esm`, `Dawnguard.esm`, `HearthFires.esm` and `Dragonborn.esm`. (Ideally, you should have pure Vanilla version installed.)

2. Calculate test coverage (Windows-only):
   
   Install [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage/releases) and then:
   ```sh
   cmake .. -DCPPCOV_PATH="C:\Program Files\OpenCppCoverage"
   ctest -C Debug --verbose
   ```
   These commands would re-generate project files with coverage enabled and run tests. Coverage report would be in `build/__coverage`.

## License

Use of this source code is subject to GPLv3. (See `LICENSE` for more information)

<!-- CONTACT -->
## Contact

Leonid Pospelov - Pospelov#3228 - pospelovlm@yandex.ru

Project Link: [https://github.com/skyrim-multiplayer/skymp](https://github.com/skyrim-multiplayer/skymp)
