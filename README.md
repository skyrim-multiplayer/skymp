# SkyMP

[![Discord Chat](https://img.shields.io/discord/699653182946803722?label=Discord&logo=Discord)](https://discord.gg/k39uQ9Yudt) 
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)](CONTRIBUTING.md)

SkyMP is an open-source multiplayer mod for Skyrim ⚡

SkyMP is built on top of the [SkyrimPlatform](docs/docs_skyrim_platform.md) - a tool to create Skyrim mods with TypeScript and Chromium. 🚀

This repo hosts all sources to ease local setup and contributing. See [CONTRIBUTING](CONTRIBUTING.md) for build instructions.

![image](skymp.jpg)

### Getting Started

If you just want to play multiplayer, [download the client]().

You can join our test server called [SweetPie]() or [host your own]().

A list of the servers online can be retrieved via API: https://skymp.io/api/servers

### What's Synced

- [x] Player movement and animation sync
- [x] Appearance: race, headparts, tints, etc
- [x] PvP sync: melee damage, attributes, death
- [x] Inventory sync: equipment, containers, craft, etc
- [x] Limited scripts sync (WIP) - we have our own Papyrus engine
- [ ] Who knows what comes next? We work on synchronizing all Skyrim game mechanics, see [ROADMAP](ROADMAP.md).

### Multiplayer Features

- Mostly server-controlled game state - you can't cheat everything
- Store your world in plain files or MongoDB
- Customize your server with TypeScript or Papyrus scripting
- Use esp/esm mods, just ensure both client and server load order are the same

### Terms of Use

SkyMP is free software. Most of SkyMP parts are distributed under GPLv3 and/or AGPLv3. Essentially, this means that you are free to do almost exactly what you want with the program, including distributing it among your friends, making it available for download from your web site, selling it (either by itself or as part of some bigger software package), or using it as the starting point for a software project of your own.

The only real limitation is that whenever you distribute SkyMP in some way, you must always include the full source code, or a pointer to where the source code can be found. If you make any changes to the source code, these changes must also be made available under the GPL.

Top level folders of this repo represent subprojects and each subproject have it's own license. For full details, read the copy of the license found in the file named `<project_name>/LICENSE.md` (for example, `skymp5-server/LICENSE.md`).

It seems that you also technically can write a proprietary remake of skymp5-client or skymp5-front to use with GPL SkyMP parts as they are not linked directly. Same for other subprojects. But for a reason, I'd say it's a way to disappointment if you ask me.

We in SkyMP [link our AGPLv3 and GPLv3 components](https://opensource.stackexchange.com/questions/8432/is-gpl-linking-to-agpl-possible).

If you'd like to do something controversial feel free to contact us for consultation. But no, we wouldn't allow you closed-source server/client forks since we respect the contributors and their copyright.

SkyMP is not affiliated with or endorsed by Bethesda Softworks, ZeniMax Media or other rightsholders. Any trademarks used belong to their respective owners.
