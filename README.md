# SkyMP

[![Discord Chat](https://img.shields.io/discord/699653182946803722?label=Discord&logo=Discord)](https://discord.gg/MsMSfUjtDp) 
[![PR's Welcome](https://img.shields.io/badge/PRs%20-welcome-brightgreen.svg)](CONTRIBUTING.md)
[![Players](https://skymp-badges.vercel.app/badges/players_online.svg)](https://discord.gg/MsMSfUjtDp) 
[![Servers](https://skymp-badges.vercel.app/badges/servers_online.svg)](https://discord.gg/MsMSfUjtDp)

SkyMP is an open-source multiplayer mod for Skyrim ⚡

SkyMP is built on top of the [SkyrimPlatform](docs/docs_skyrim_platform.md) - a tool to create Skyrim mods with TypeScript and Chromium. 🚀

This repo hosts all sources to ease local setup and contributing. See [CONTRIBUTING](CONTRIBUTING.md) for build instructions.

### Terms of Use

See [TERMS.md](TERMS.md). TL;DR disclose the source code of your forks.

Third-party code licenses can be found in [THIRD_PARTY_LICENSES](THIRD_PARTY_LICENSES).

### Development with GitHub Codespaces

[![Create Codespace](https://img.shields.io/badge/Codespace-Launch-blue?logo=github)](https://github.com/codespaces/new?repo=skyrim-multiplayer/skymp&ref=main)

### Instructions for new servers

1) Fork this repo into your own directory. I recommend creating an organization through github instead of your personal account. 

2) Go to the actions tab in your new repo, and activate workflows. I recommend for now, disabling all but the one you need.
For windows users, make sure PR Windows Flatrim is activated, for Linux make sure PR Linux is activated.
Formatting Check is a useful one to keep on. It'll run linter on your scripts which will find errors for you.

3) Clone your new repo onto your server's desktop. This can be done with the github desktop app if you're unfamiliar with the git command line. 
This guide assumes you have a windows VPS to run your server off of, with an account called "Administrator." If you don't adjust the .bat lines accordingly.

4) Update your README.md file and push the update to github. 
This will cause the workflow to trigger. Warning, this COULD take a couple hours, but it will build your server for you.

5) Download the server-dist artifact from the workflow build, then extract it in C:\Users\Administrator\Desktop\SkyMP\builds\dist\server
Note, this is the default path. You will need to make the builds\dist\server folders. If your username isn't Administrator, or if you're not using windows, correct accordingly.

6) In server-settings.json, tweak the load order to match the path to your current install of Skyrim.
You can also add in any mods here, or tweak/add any settings from the server settings document. 

7) Install NSSM. There's a bat file located in the base directory, just run it and it'll do it for you.
NSSM turns your SkyMP server into a service that runs automatically, such as after a crash or server restart. 
Note: The bat file assumes your path is C:\Users\Administrator\Desktop\SkyMP If this is not the case, please edit the bat file to the correct path.