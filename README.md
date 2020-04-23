# Skyrim Platform

A modding tool for Skyrim allowing writing scripts with JavaScript/TypeScript.

### Build pre-requirements:
 1. Microsoft Visual Studio 2019
 2. .NET Framework SDK 4.8

### End-user dependencies:
 1. Skyrim Special Edition
 2. [SKSE64 ](https://skse.silverlock.org/beta/skse64_2_00_17.7z)
 3. [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

### Building:
```batch
git submodule update --init
deps.bat
gen.bat
```
Run `gen.bat` every time you modify any CMake stuff.
Go to `build/_platform_se` and open `.sln` file.

### Configuring dev_service
`dev_service` allows you to restart the game automatically each time changes in C++ code detected.

Install dependencies:
```batch
cd src/dev_service
npm i
```
Start:
```batch
npm run dev
```
After started, `config.js` will be generated:
```js
module.exports = {
  'SkyrimSEFolder': '...',
  'SkyrimMultiplayerBinaryDir': '...'
};
```
Change `SkyrimSEFolder` to path to your `Skyrim Special Edition` folder (like `C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition` if you are using steam version).
Change `SkyrimMultiplayerBinaryDir` to path to your `build` dir (`%RepoRoot%/build`).
