# Server Ports Usage

Currently, the server uses three ports to keep all systems work. This page describes the role of each port, default values, etc.

## Main Port

Used to perform synchronization and other basic networking.

- Protocol is UDP
- Default value is 7777
- Configurable via [configuration file](docs_server_configuration_reference.md) or [command line API](docs_server_command_line_api.md)

## UI Port

Used by the embedded browser to access HTML/CSS/JS and other assets.

- Protocol is HTTP
- Configurable via `uiPort` in `server-settings.json`
- If `uiPort` is omitted, server uses the same value as `port`
- `uiListenHost` can be used to control bind address for this HTTP listener

## WebPack DevServer Port

If you run the WebPack dev server and the skymp server on the same machine, the skymp server would proxy UI requests to the WebPack dev server.
This feature allows you to use frontend live reload to test game systems.

- Development probe target is `http://localhost:1234`
- When available, `/ui/*` requests are proxied to that dev server
- Other API/asset requests are served by the built-in server app

## Chromium DevTools

Actually, this is not a serverside port.
You may need to know that the embedded browser exposes port 9000 for remote DevTools.
Just type `localhost:9000` in your _real_ browser to open Chromium DevTools for the in-game browser.
