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
Generally, your domain should have A records (for api.website and dashboard.website) pointing to your VPS' IP address.

2) Fork this repo into your own directory. I recommend creating an organization through github instead of your personal account. 

3) Go to the actions tab in your new repo, and activate workflows. I recommend for now, disabling all but the one you need.
For windows users, make sure PR Windows Flatrim is activated, for Linux make sure PR Linux is activated.
Formatting Check is a useful one to keep on. It'll run linter on your scripts which will find errors for you.

4) Clone your new repo onto your server's desktop. This can be done with the github desktop app if you're unfamiliar with the git command line. 
This guide assumes you have a windows VPS to run your server off of, with an account called "Administrator" and the repo located on the desktop.
If you don't adjust the .bat lines accordingly.

5) Update your README.md file and push the update to github. 
This will cause the workflow to trigger. Warning, this COULD take a couple hours, but it will build your server for you.

6) Download the server-dist artifact from the workflow build, then extract it in C:\Users\Administrator\Desktop\SkyMP\builds\dist\server
Note, this is the default path, change as needed. 

7) Follow the instructions inside of \SkyMP\builds\dist\server\README.md
It will guide you almost completely, including a full server-settings.json to fill out. 

8) Go to \SkyMP\skymp5-backend\ and run Setup-Backend.vat followed by build-client.bat
Both of these will do everything for you. I recommend looking at the README.md for the launcher so you can configure it to your liking. 

9) Go to \SkyMP\skymp5-launcher\ and run the build-launcher.bat file
This will give you your .exe to distribute to your players. Simply zip it up, and upload it to any CDN of your choosing (such as discord, or your website).

10) Finally, install nginx on your machine with the final script in the root directory.
This script also will run win-acme to create certs for your API and Dashboard, making it a true server. 
Big note, you must edit this bat file, and the two conf files in SkyMP/deploy/nginx
I tried to leave it pretty simple, just fill out where it asks you to.

If you need anything further, read the documentation already provided in \SkyMP\docs
It literally contains everything else.

If both services are running and your launcher connects, congrats!
Have fun and enjoy your server! 
The rest of your journey involves developing it and branching it off from the rest. Good luck!
