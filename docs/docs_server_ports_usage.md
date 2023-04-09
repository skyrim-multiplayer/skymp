# Server Ports Usage

Currently, the server uses three ports to keep all systems work. This page describes the role of each port, default values, etc.

## Main Port

Used to perform synchronization and other basic networking.

- Protocol is UDP
- Default value is 7777
- Configurable via [configuration file](docs_server_configuration_reference.md) or [command line API](docs_server_command_line_api.md)

## UI Port

Used by the embedded browser to access HTML/CSS/JS and other assets.

- Protocol is HTTPS
- Default value is 3000
- Non-configurable
- Equals `(Main Port + 1)` if its value is non-default

## WebPack DevServer Port

If you run the WebPack dev server and the skymp server on the same machine, the skymp server would proxy UI requests to the WebPack dev server.
This feature allows you to use frontend live reload to test game systems.

- Non-configurable, assumed to always be 1234

## Chromium DevTools

Actually, this is not a serverside port.
You may need to know that the embedded browser exposes port 9000 for remote DevTools.
Just type `localhost:9000` in your *real* browser to open Chromium DevTools for the in-game browser.
