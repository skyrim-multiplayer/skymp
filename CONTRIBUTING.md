# Contributing

Contributions are what make the open source community such an amazing place to be learn, inspire, and create. Contributions are welcome and will be fully credited.

We welcome all contributions, there is no minor things to contribute with, even one letter typo fixes are welcome.

The only things we require is to test thoroughly, maintain code style and keeping documentation up-to-date.

Also, accepting and agreeing to release any contribution under the same license.

## Building From Source

You can find instructions on setting up the project locally below. To get a local copy up and running follow these simple example steps. You need ~4 GB RAM, ~22 GB on your hard drive and some free time.

## Prerequisites

### Common

These tools required regardless of your system:

* 64-bit [NodeJS](https://nodejs.org/en/download/) 17.x *(may also work for older versions)*
* [Yarn](https://yarnpkg.com/getting-started/install): `npm install --global yarn`
* [CMake 3.18.2](https://cmake.org/download/) or higher

### Windows

Before your start make sure that your system meets the conditions:

* Windows 7 or higher *([Windows 10](https://www.microsoft.com/en-us/software-download/windows10) is recommended)*
* [Visual Studio 2019/2022](https://visualstudio.microsoft.com/downloads/)
* .NET Framework SDK at 4.6.0 or higher *(Visual Studio Installer -> .NET desktop development)*
* Python 3.9.x 64bit or 32bit *(Visual Studio Installer -> Idividual components -> search for python)*

### Linux

You can build and run server and unit-tests on Linux.
As Skyrim has no native Linux version, client can only be built using MSVC,
but then can be run with Proton (though some crashes can occur on SP startup
and it can be tricky to get Skyrim itself to work with non-ASCII text, for example).

* Ubuntu 18.04 or 20.04. Other distros are not tested or are expected to fail:
  * Alpine Linux doesn't work
  * Arch-based distros also [won't be able to run the server](https://github.com/chakra-core/ChakraCore/issues/6613)
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
Also you can use containers to build and run server. More info can be found in the next section.

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

### Linux

On Linux, there might be some tricky dependency issues. To work around them,
we recommend you to use a wrapper script `build.sh`. It will create a temporary
directory and add some aliases to `PATH`.

If you don't wish to build all the dependencies by yourself, or have an unsupported distro,
you can use [a Docker image with preinstalled dependencies](https://hub.docker.com/r/skymp/skymp-vcpkg-deps):

```sh
. misc/github_env_linux; docker run -it --rm -v "$PWD:$PWD" -w "$PWD" -u "`id -u`:`id -g`" \
    $SKYMP_VCPKG_DEPS_IMAGE bash
# ... or go rootless!
. misc/github_env_linux; podman run -it --rm -v "$PWD:$PWD" --security-opt label=disable -w "$PWD" \
    -e VCPKG_DEFAULT_BINARY_CACHE=/home/skymp/.cache/vcpkg/archives \
    $SKYMP_VCPKG_DEPS_IMAGE bash
```
`--security-opt label=disable` is used for users who have SELinux enabled and source code is located somewhere inside home directory.
Check `podman run` [documentation](https://docs.podman.io/en/latest/markdown/podman-run.1.html) for more information.

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
   Also you can add `--parallel $(nproc)` to use all available CPU threads for build system

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

## Pull Requests

- **Your branch must be buildable** - The project's build system must be able to build repo with your changes.

- **Create feature branches** - Don't ask us to pull from your master branch.

- **One pull request per feature** - If you want to do more than one thing, send multiple pull requests.

- **Give a reasonable name to your PR** - It would help to process your PR.

- **Format your C++ code** - Make sure that you format your C++ code with rules in `.clang-format`.

- **Format your TypeScript code** - For now, keep the same formatting as modified documents have (`RMB -> Format Document` in VS Code). Double quotes (" ") are used to represent a string in TypeScript code.

- **Prefer spaces over tabs** - To prevent inconsistent formatting. Applies to all languages.

- **Commit naming doesn't matter for you** - There are no strict rules in commit naming you must follow. Maintainers are responsible for fixing commit names.

- **Keep Git history clean** - Push only necessary changes. Change file paths only if necessary.

- **Remove code instead of turning it into comments** - Let Git manage history for you instead.

- **No offensive language in code** - Let's code and comment in ways that do not hurt anyone.

- **Resolve merge conflicts** - Your branch should not conflict with main branch.

## Step By Step

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/my-awesome-feature`)
3. Commit your Changes (`git commit -m 'Add some my-awesome-feature'`)
4. Push to the Branch (`git push origin feature/my-awesome-feature`)
5. Open a Pull Request

**Happy coding**!
