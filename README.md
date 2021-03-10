<!-- PROJECT LOGO -->
<br />
<p align="center">
  <a href="https://skymp.io">
    <img src="skymp.jpg" alt="Logo" width="200" height="200">
  </a>

  <h3 align="center">SKYRIM PLATFORM</h3>

  <p align="center">
    A modding tool for Skyrim allowing writing scripts with JavaScript/TypeScript.
    <br />
    <a href="https://pospelovlm.gitbook.io/skyrim-multiplayer-docs/docs_skyrim_platform"><strong>Explore the docs »</strong></a>
    <br />
    <br />
    <a href="https://www.youtube.com/watch?v=b-kJte3AMYA">View Demo</a>
    ·
    <a href="https://github.com/skyrim-multiplayer/skyrim-platform/issues">Report Bug</a>
    ·
    <a href="https://github.com/skyrim-multiplayer/skyrim-platform/issues">Request Feature</a> 
    ·
    <a href="https://discord.gg/NKtwpuGCHD">Join Discord</a>
  </p>
</p>

```typescript
printConsole('Hello Platform');

on('update', () => {
  const gold = Game.getForm(0xf);
  const target = Game.getDialogueTarget();
  
  if (target && Game.getPlayer().getItemCount(gold) >= 100) {
    Game.getPlayer().removeItem(gold, 100, true, target);
    Debug.notification('You have just paid to an NPC');
  }
});
```

<!-- ABOUT THE PROJECT -->
## About The Project

SkyrimPlatform is a new scripting system for Skyrim Special Edition that allows you to develop mods in modern TypeScript (JavaScript) as an alternative to Papyrus.

## Features

### Hot Reload
Scripts on SkyrimPlatform can be added/modified/removed without restarting the game, which greatly improves the development experience. You can literally minimize the game, make a change to the script, and maximize it again.

### Speed
The code is significantly faster than Papyrus. You can call any function every game frame. The speed is limited only by the performance of the user's PC. The overhead of the JavaScript engine itself is reduced through various optimizations.

### Global Scripts
SkyrimPlatform allows you to create global scripts that are not attached to a specific object and run continuously. Including, during a pause and in the main menu. There is no need to create .esp.

### SKSE Compatibility
SkyrimPlatform is technically an SKSE plugin written in C++. When writing scripts, you can use standard SKSE functions and functions from SKSE plugins.

### ES6 Modules
SkyrimPlatform supports imports/exports thanks to its own module loader which mimics [systemjs](https://github.com/systemjs/systemjs).

### In-game Browser
SkyrimPlatform

## Built With

* [ChakraCore](https://github.com/chakra-core/ChakraCore)
* [CEF](https://bitbucket.org/chromiumembedded/cef)
* [Frida](https://frida.re/)


<!-- GETTING STARTED -->
## Installation

You can download a latest build [here](https://skymp.io/nightly/index.html) An archive contains the Skyrim Platform distribution and the plugin example.

## Building From Source

You can find instructions on setting up the project locally below.
To get a local copy up and running follow these simple example steps.

### Prerequisites

Before your start make sure that your system meets the conditions:

* Windows 7 or higher *([Windows 10](https://www.microsoft.com/en-us/software-download/windows10) is recommended)*
* [Visual Studio 2019](https://visualstudio.microsoft.com/downloads/)
* .NET Framework SDK at 4.6.0 or higher *(Visual Studio Installer -> .NET desktop development)*
* [NodeJS 12.x](https://nodejs.org/en/download/) or higher
* [CMake 3.20](https://cmake.org/download/) or higher

### Configuring Project

1. Clone the repo, including submodules
   ```sh
   git clone --recursive https://github.com/skyrim-multiplayer/skyrim-platform.git
   ```
2. Make a build directory (used for project files, cache, artifacts, etc)
   ```sh
   mkdir build
   ```
3. Generate project files with CMake
   ```sh
   cd build
   cmake ..
   ```

### Building

1. Open `build/platform_se.sln` with Visual Studio, then `Build -> Build Solution`
2. Form a SkyrimPlatform distribution with dev_service
   ```sh
   cd tools/dev_service
   npm run pack
   ```
   Normally you would see something like
   ```
   > dev_service@1.0.0 pack c:\projects\skyrim-platform\tools\dev_service
   > cross-env DEV_SERVICE_ONLY_ONCE=yes DEV_SERVICE_NO_GAME=yes node .

   Dev service started
   Binary dir is 'c:\projects\skyrim-platform\build'
   Source dir is 'c:\projects\skyrim-platform'
   Skyrim Platform Release x64 updated.
   ```
3. Copy contents of `tools/dev_service/dist` folder to the Skyrim SE root

### Building in Watch Mode

Copying binaries and restarting the game on each compilation may be boring and frustrating. Let's turn on the watch mode letting dev_service doing its job.

1. Create `tools/dev_service/config.js` with `SkyrimSEFolder` specified like in the example below
   ```js
   module.exports = {
     SkyrimSEFolder:
       "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skyrim Special Edition",
   };
   ```
2. Launch dev_service in watch mode
   ```sh
   cd tools/dev_service
   npm run watch
   ```
After doing these steps you would see that dev_service restarts your game automatically every time you compile Skyrim Platform.

<!-- USAGE EXAMPLES -->
## Real World Usage

Since Skyrim Platform is built as a part of the [Skyrim Multiplayer](https://skymp.io) project, [skymp5-client](https://github.com/skyrim-multiplayer/skymp5-client) is built in top of this tool. It's an open-source project so you are free to investigate. A good example of self-contained system is in the [worldCleaner.ts](https://github.com/skyrim-multiplayer/skymp5-client/blob/6013e4f8c28a8cb8621a2b5543037b63164dfd7a/src/front/worldCleaner.ts)

_For more information, please refer to the [Documentation](https://pospelovlm.gitbook.io/skyrim-multiplayer-docs/docs_skyrim_platform)_



<!-- ROADMAP -->
## Roadmap

See the [open issues](https://github.com/skyrim-multiplayer/skyrim-platform/issues) for a list of proposed features (and known issues).



<!-- CONTRIBUTING -->
## Contributing

Contributions are what make the open source community such an amazing place to be learn, inspire, and create. Any contributions you make are **greatly appreciated**.

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request



<!-- LICENSE -->
## License

Use of this source code is subject to the Skymp Service Agreement provided on the Skyrim Multiplayer website at the following URL: https://skymp.io/terms.html
(See `LICENSE` for more information)



<!-- CONTACT -->
## Contact

Leonid Pospelov - Pospelov#3228 - pospelovlm@yandex.ru

Project Link: [https://github.com/skyrim-multiplayer/skyrim-platform](https://github.com/skyrim-multiplayer/skyrim-platform)
