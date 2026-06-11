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

1) Set up your server and website.
You will need a domain, website, VPS, and knowledge on how to edit DNS records to create the API and Dashboard.
This I cannot provide too much documentation on, but any decent provider worth their salt should be able to assist you.

3) Fork this repo into your own directory. I recommend creating an organization through github instead of your personal account. 

4) Go to the actions tab in your new repo, and activate workflows. I recommend for now, disabling all but the one you need.
For windows users, make sure PR Windows Flatrim is activated, for Linux make sure PR Linux is activated.
Formatting Check is a useful one to keep on. It'll run linter on your scripts which will find errors for you.

5) Clone your new repo onto your server's desktop. This can be done with the github desktop app if you're unfamiliar with the git command line. 
This guide assumes you have a windows VPS to run your server off of, with an account called "Administrator" and the repo located on the desktop.
If you don't adjust the .bat lines accordingly.

6) Update your README.md file and push the update to github. 
This will cause the workflow to trigger. Warning, this COULD take a couple hours, but it will build your server for you.

7) Download the server-dist artifact from the workflow build, then extract it in C:\Users\Administrator\Desktop\SkyMP\builds\dist\server
Note, this is the default path, change as needed. 

8) Follow the instructions inside of \SkyMP\builds\dist\server\README.md
It will guide you almost completely, including a full server-settings.json to fill out. 

9) Follow the instructions inside of \SkyMP\skymp5-backend\README.md
It will guide you almost completely, along with the .env settings and any changes you need to make

10) Follow the instructions inside of \SkyMP\skymp5-launcher\README.md
It will guide you almost completely, along with making a build you can share on your website

11) If you need anything further, read the documentation already provided in \SkyMP\docs
It literally contains everything else.

Have fun and enjoy your server!
