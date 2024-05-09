# Skyrim Platform

Create Skyrim mods in JavaScript/TypeScript.

If you're seeing this, you're likely interested in creating a Skyrim mod in TypeScript or using one.

Here is everything you need to know:

## Supported Games

- Steam latest (currently Skyrim SE/AE 1.6.1170)
- Skyrim SE/AE 1.6.640
- Skyrim SE 1.5.97.0.8 (Deprecated, not accepting bugs anymore)

## For Mod Creators

In general why is it good:

- **The power of JavaScript.** JSON, RegExp, classes, functional programming, npm packages, whatever.
- **Hotreload.** SP watches plugins by default. Ctrl+S, Alt+Tab, and you can test your changes in-game.
- **IDE support.** You'll see many errors compile-time.
- **Promises and async/await support.** Supported for async Papyrus functions like `Utility.wait`, `ObjectReference.setPosition`, etc.
- **Safety.** SP protects you from null pointers, threadding issues, etc. 
- **SKSE compatibility.** You can call 3rd party SKSE functions natively from JS/TS.
- **Mod managers support.** Nothing goes to Skyrim root.
- **Chromium Embedded Framework.** You can build ingame UI in React, VueJS, etc.


Some Drawbacks:

- Limited MCM support.
- Inadequate savegame support: managing the game state can be tricky.
- Users must download Skyrim Platform to access your mod.
- Animations in Chromium eat FPS.
- Multiple plugins with Chromium-based UIs are not properly supported. Workarounds needed.

You'll love Skyrim Platform, especially if you're:

### ...a JavaScript Developer with No Modding Background
Your favorite tech stack is now available for Skyrim modding.

### ...a Hyperactive Person
The hot-reload feature lets you prototype and iterate rapidly with an extremely short feedback loop. It's incredibly fast compared to Papyrus/C++ development.

Skyrim Platform is an excellent starting point for Skyrim programming. If desired, you can transition to C++ later, as many people do.

Checkout our [docs](https://github.com/skyrim-multiplayer/skymp/blob/main/docs/docs_skyrim_platform.md) and [plugin example](https://github.com/skyrim-multiplayer/skymp/tree/main/skyrim-platform/tools/plugin-example). Good luck building!

## For end-users of SP-based mods

The latest release is always on our [Nexus Mods page.](https://www.nexusmods.com/skyrimspecialedition/mods/54909)

I must admit, using SP plugins isn't as satisfying as creating them. Please consider these points:

- **Please use [SSE Engine Fixes](https://www.nexusmods.com/skyrimspecialedition/mods/17230).** Otherwise, Chromium process will hang in your task manager after game exit.
- **SkyrimPlatform is a plugins runner.** If it drops your FPS, then most likely you're running an unoptimized SkyrimPlatform plugin. Not so much we can do on our side.
- **SkyrimPlatform conflicts with many mods.** Be careful.
- We're not accepting bugreports for 1.5.97 version of SkyrimPlatform.
- **Updating deleting SkyrimPlatform on a current save might break your save.** You need to start a new game.
- **In most cases you can repair your savegames.** See [sticked NexusMods posts](https://www.nexusmods.com/skyrimspecialedition/mods/54909?tab=posts).

## Future plans

- **Fix bugs, improve compatibility.** SkyMP as a project and I will dedicate what we can to fix SP bugs.
- **Migrate to NodeJS.** Right now [ChakraCore](https://github.com/chakra-core/ChakraCore) powers our mod.
- **Improve game functions performance.** There are ES6 Proxies under the hood, we have to optimize it out.
- **Documentation.** Document undocumented APIs. Contributions welcome!
- **Support SkyMP as a Game**: We're developing a multiplayer mod for Skyrim that requires its own scripting API. To meet this need, we've decided to partially implement SkyrimPlatform APIs on the SkyMP server.

## SkyMP

We're building a FiveM/TES3MP like mod for Skyrim. We'll announce that when we're ready.

## FAQ

Q: How do I distribute my plugins?
A: SkyrimPlatform plugins are compiled into JS files (see tsconfig.json in plugin example). So you need to distribute your-plugin-name.js. Users will need to put it into Data/Platform/Plugins.

Q: SKSE plugins typically have ini configuration. How can users configure my SkyrimPlatform plugin?
A: There is a settings API (see docs). Settings are also stored Data/Platform/Plugins. Settings file suffix is "-settings.txt". Your settings file would probably be called your-plugin-name-settings.txt or something like this.

Q: How do I create a plugin?
A: See Data/Platform/plugin-example in the archive. It's an example plugin. This folder includes README.md describing how to use our plugin example.

Q: How do I get hot reload to work?
A: If you followed instructions from Data/Platform/plugin-example/README.md, just Ctrl+S in your favorite code editor.
