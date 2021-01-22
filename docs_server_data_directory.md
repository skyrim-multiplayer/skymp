# Server Data Directory

Data directory of the server (`"./data"` by default) contains different kinds of game resources, including mods, UI, etc.

## Vanilla Skyrim SE Mods

Should be placed into the root of the data directory and added to `"loadOrder"` in `server-settings.json`.
All assets required for the game should be packed into an archive in .bsa format by Creation Kit or community tools.
That archive must have the same name with related mod (i.e. `"FooBar.bsa"` for `"FooBar.esp"`).
Please note that currently .bsa archives are used only on the client-side. If you want scripts to be working on the server, place them into the `scripts` subdirectory.

## UI

`ui` subdirectory is used for storing your server's user interfaces. It should contain `index.html` and other bundled files.

## Manifest

The server generates `manifest.json` during startup. Do not modify that file, consider modifying `server-settings.json` instead.

## _libkey.js

Data directory also contains `_libkey.js`. This file is used as a script embedded into the CEF page in-game.
You probably don't want to modify this. Otherwise, there is usually a more reliable way to do things.
