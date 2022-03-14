# SkyMP

[![Discord Chat](https://img.shields.io/discord/699653182946803722?label=Discord&logo=Discord)](https://discord.gg/k39uQ9Yudt) 
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)]()

SkyMP is an open-source multiplayer mod for Skyrim ⚡

Built on top of the [SkyrimPlatform](https://www.nexusmods.com/skyrimspecialedition/mods/54909) - a tool to create Skyrim mods with TypeScript and Chromium. 🚀

This repo hosts all sources to ease local setup and contributing. See CONTRIBUTING for build instructions.

![image](https://user-images.githubusercontent.com/37947786/158084241-7057ea3e-3644-460c-9ad1-9f9b63859e15.png)

### Multiplayer Features

- Mostly server-controlled game state - you can't cheat everything
- Store your world in plain files or MongoDB
- Customize your server with TypeScript or Papyrus scripting
- Use esp/esm mods, just ensure both client and server load order are the same

### What's Synced

- [x] Player movement and animation sync
- [x] PvP sync: melee damage, attributes, death
- [x] Inventory sync: equipment, containers, etc
- [x] Limited scripts sync (WIP) - we have our own Papyrus engine
- [ ] Who knows what comes next? We work on synchronizing all Skyrim game mechanics, see ROADMAP.
