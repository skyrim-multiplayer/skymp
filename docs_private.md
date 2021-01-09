# Launching your server

As you already know Skyrim Multiplayer is releasing public server builds. Here is an instruction on running your own server.

## Windows

1. Create a new directory for server installation.
2. Download the latest server build from the project's [website](https://skymp.io/).
3. Extract everything into a previously created directory.
4. Go to your Skyrim Special Edition installation directory \(typically, `C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition`\). Then go to the`Data` directory. You need to copy `Dawnguard.esm`, `Dragonborn.esm`, `HearthFires.esm`, `Skyrim.esm`, and `Update.esm` to the `data` subdirectory of the server directory.
5. \(Optional\) If you want vanilla Papyrus scripts to run, download an [archive](https://github.com/skyrim-multiplayer/skymp5-binaries/releases/download/scripts/scripts.7z) with scripts and extract everything into a `data/scripts` subdirectory of the server directory. Otherwise, you will get warnings but the server will keep working without Papyrus support.

## Other operating systems

Currently, we do not support non-Windows server builds.

