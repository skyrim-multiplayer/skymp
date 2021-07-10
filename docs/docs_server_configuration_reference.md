# Server Configuration Reference

The recommended way to configure the server is setting up all required values in `server-settings.json`. It's standard JSON without C-style comments support. See also [Server Command Line API](docs_server_command_line_api.md).

## name

Server's name. Displayed on skymp.io and in launcher.

```json
{
  // ...
  "name": "My Server"
  // ...
}
```

## ip

This IP-address would be used by player clients to connect to your server. Do not try to type `"0.0.0.0"`, just remove this option from document if you want to use your current public IP.

```json
{
  // ...
  "ip": "127.0.0.1"
  // ...
}
```

## port

This port would be used by player clients to connect to your server. At the current version of Skyrim Multiplayer servers use multiple ports and different protocols to manage different sorts of packets. See [Server Ports Usage](docs_server_ports_usage.md) page to learn more.

```json
{
  // ...
  "port": 7777
  // ...
}
```

## maxPlayers

Sets player limit of the server. Visible in launcher and on skymp.io.

```json
{
  // ...
  "maxPlayers": 108
  // ...
}
```

## dataDir

Contains relative path to a directory with vanilla Skyrim data (mods in esp/esm format, compiled Papyrus scripts in pex format).

```json
{
  // ...
  "dataDir": "data"
  // ...
}
```

## loadOrder

A list of esp/esm files which would be loaded by the server during startup in the same order as Skyrim SE loads them. The server looks for files in `${dataDir}` directory.

```json
{
  // ...
  "loadOrder": [
    "Skyrim.esm",
    "Update.esm",
    "Dawnguard.esm",
    "HearthFires.esm",
    "Dragonborn.esm"
  ]
  // ...
}
```

## databaseDriver

Name of a database driver which would be used to store server data. `sqlite` by default. There are also related options like `"databaseName"`. See [Database Drivers](docs_database_drivers.md) page to learn more.

```json
{
  // ...
  "databaseDriver": "sqlite"
  // ...
}
```

## reloot

A time before a game object restores its original state in milliseconds. Unlike Skyrim SE, Skyrim Multiplayer doesn't have a built-in Cell Reset mechanism. The server resets every object in the world every hour instead. With this option, you can change this time interval for every kind of game object. `"CONT"`, for example, means "Container" - chests, barrels, etc. See "record types" on [UESP](https://en.uesp.net/wiki/Skyrim_Mod:Mod_File_Format).

```json
{
  // ...
  "reloot": {
    "FLOR": 86400000,
    "TREE": 86400000,
    "AMMO": 86400000,
    "ARMO": 86400000,
    "BOOK": 86400000,
    "INGR": 86400000,
    "ALCH": 86400000,
    "SCRL": 86400000,
    "CONT": 86400000,
    "SLGM": 86400000,
    "WEAP": 86400000,
    "MISC": 86400000
  }
  // ...
}
```

## gamemodePath

Contains a relative or an absolute path to a file or directory with a gamemode.
Searches for `index.js` if a directory specified.

```json
{
  // ...
  "gamemodePath": "gamemode.js"
  // ...
}
```

## isPapyrusHotReloadEnabled

A boolean setting that enables to turn on or turn off hot reload for compiled Papyrus scripts (.pex)

```json
{
  // ...
  "isPapyrusHotReloadEnabled": false
  // ...
}
```

## locale

The name of a localizaiton file in `data/localization` that would be used by `M.GetText` Papyrus function (without extension).

```json
{
  // ...
  "locale": "ru-RU"
  // ...
}
```