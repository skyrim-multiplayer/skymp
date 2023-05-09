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

`"versionMajor"` is a major version of the Manifest, currently, `1`.
`"mods"` is an array of objects with fields `"crc32"`, `"filename"` and `"size"`.
`"loadOrder"` is a load order of mods (taken from `server-settings.json` directly).
