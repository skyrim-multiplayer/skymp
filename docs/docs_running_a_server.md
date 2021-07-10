# Running a Server

As you already know Skyrim Multiplayer is releasing public server builds. Here is an instruction on running your own server.

## Installation

### Windows

The server requires `Windows 8.1` / `Windows Server 2012` or higher. It may still launch on older operating systems, but correct work isn't guaranteed.

You obviously need to have 64-bit Windows version since the server is 64-bit program.

You are able to build whole project from sources. Server build would be in `build/dist/server`

### Linux

Only Windows builds are supported currently.

## Configuration

Once you install the server, you should be able to launch it. We distribute server builds with `server-settings.json` included, but its default values are only usable to verify that server works. After launching the server you will see a server called `My Server` in skymp launcher. You also will be able to connect, but players from the Internet will not. You need to change the `ip` field in `server-settings.json` to get this functionality to work.

```json
{
  "dataDir": "data",
  "loadOrder": [
    "Skyrim.esm",
    "Update.esm",
    "Dawnguard.esm",
    "HearthFires.esm",
    "Dragonborn.esm",
    "FarmSystem.esp"
  ],
  "ip": "127.0.0.1", // <=
  "name": "My Server"
}
```

- You may find out your public IP here http://ipv4bot.whatismyipaddress.com
- You need to have ports open. Talk to your Internet provider support if you want to open ports. Status of each port can be checked here https://www.yougetsignal.com/tools/open-ports/. You can learn about ports that are really used by the server on [Server Configuration Reference](docs_server_configuration_reference.md) page or to simplify think that it may use any of available ports and protocols.
- If you use `LogMeIn Hamachi` or similar software then just type an IP address you got assigned from it. Your friends who share a "local" network with you will be able to connect, players from the Internet will not.
