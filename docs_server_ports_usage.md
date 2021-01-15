# Server Ports Usage

Currently, the server uses three ports to keep all systems work. This page describes the role of each port, default values, etc.

## Main Port

Used to perform synchronization and other basic networking.

- Protocol is UDP
- Default value is 7777
- Configurable via [configuration file](!docs_server_configuration_reference.md) or [command line API](!docs_server_command_line_api.md)

## UI Port

Used by the embedded browser to access HTML/CSS/JS and other assets.

- Protocol is HTTPS
- Default value is 3000
- Non-configurable
- Equals `(Main Port + 1)` if its value is non-default

## WebSocket Communication Port

Used by the embedded browser to communicate with the backend.

- Protocol is WebSocket
- Default value is 8080
- Non-configurable
- Equals `(Main Port + 2)` if its value is non-default
