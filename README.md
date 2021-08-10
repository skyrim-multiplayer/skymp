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

You can find instructions on setting up the project locally below. To get a local copy up and running follow these simple example steps. You need ~10 GB on your hard drive and some free time.

## Prerequisites

### Common

These tools required regardless of your system:

* 64-bit [NodeJS](https://nodejs.org/en/download/) 12.x or higher + npm
* [CMake 3.18.2](https://cmake.org/download/) or higher

### Windows

Before your start make sure that your system meets the conditions:

* Windows 7 or higher *([Windows 10](https://www.microsoft.com/en-us/software-download/windows10) is recommended)*
* [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* .NET Framework SDK at 4.6.0 or higher *(Visual Studio Installer -> .NET desktop development)*

### Linux

Playing on Linux isn't supported currently, but a full-featured server is.

* Ubuntu *(Some other distributions may also work, but we know that Alpine doesn't)*
* Clang 11 or higher *(GCC is not supported)*

### Configuring and Building

1. Clone the repo, including submodules
   ```sh
   git clone https://github.com/skyrim-multiplayer/skymp.git
   cd skymp
   git submodule update --init --recursive
   ```
2. Make a build directory (used for project files, cache, artifacts, etc)
   ```sh
   mkdir build
   ```
3. Generate project files with CMake (replace path with your actual Skyrim SE folder)
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

4. Build with CMake:
   ```sh
   cmake --build . --config Debug
   ```
   On Windows you also can open `build/skymp.sln` with Visual Studio, then `Build -> Build Solution`.

   All build artifacts would be placed into `build/dist`.

5. Run tests (optional):
   ```sh
   ctest -C Debug --verbose
   ```
   Some tests ([ESPMTest](https://github.com/skyrim-multiplayer/skymp/blob/main/skymp5-server/cpp/unit/EspmTest.h)) require Skyrim SE data files and will be skipped if you didn't specify `-DSKYRIM_DIR`.

   In order to avoid potential errors, make sure:
   1. You have installed it using Steam and it's up to date (currently last update was on [Nov 20, 2019](https://steamdb.info/depot/489832/history/?changeid=M:8702665189575304780)). See SteamDB for [hashes](https://steamdb.info/depot/489832/?show_hashes) and [update history](https://steamdb.info/depot/489832/history/).
   2. You did not modify `Skyrim.esm`, `Update.esm`, `Dawnguard.esm`, `HearthFires.esm` and `Dragonborn.esm`. (Ideally, you should have pure Vanilla version installed.)

## License

Use of this source code is subject to GPLv3. (See `LICENSE` for more information)

<!-- CONTACT -->
## Contact

Leonid Pospelov - Pospelov#3228 - pospelovlm@yandex.ru

Project Link: [https://github.com/skyrim-multiplayer/skymp](https://github.com/skyrim-multiplayer/skymp)
