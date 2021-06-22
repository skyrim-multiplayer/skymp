# skymp5-scripts

A set of scripts for RolePlay experience in Skyrim Multiplayer.

## Getting Started

### Prerequisites

1. Have the game installed, only Skyrim Special Edition supported
2. Have the [Creation Kit installed](https://github.com/joelday/papyrus-lang/wiki/Creation-Kit#installation)
3. [Papyrus lang for VS Code](https://marketplace.visualstudio.com/items?itemName=joelday.papyrus-lang-vscode)

### Installation

1. Download and extract a latest server build. Make sure that `./data/scripts` is empty.
2. Start the server and wait for scripts download. 
3. After the server is finally started, stop it and verify that `./data/scripts` contains `.pex` files and `./data/scripts/source` contains `.psc` files.
4. In `skyrimse.ppj` change `Output` to an absolute path to server's `./data/scripts` and change `<Import>C:/projects/skymp5-server/data/Scripts/Source</Import>` to an absolute path to server's `./data/scripts/Source`.
5. Set `isPapyrusHotReloadEnabled` to true in your `server-settings.json`. Turn it off before letting the real users join the server since that feature affects the server's performance.
6. Specify a valid `gamePath` in `.vscode/tasks.json`.

### Development

You should be able to build the project with `Ctrl + Shift + B`. The build system would invoke Papyrus Compiler and copy built `.pex` files into your server's `./data/scripts` folder. SkyMP server reloads your scripts as soon as any function from your scripts is called.

### Localizing Scripts

Create a JSON file in `data/localizaiton` (i.e. `ru-RU.json`). The localization format is pretty simple. Keys are original strings, values are localized ones.
```json
{
    "Unknown command": "Неизвестная команда",
    "smiled": "улыбается"
}
```
Usage
```Papyrus
; Would be assigned to "улыбается" if ru-RU.json is used
; Defaults to an original string ("smiled") on any failure (missing file, etc)
String localizedStr = M.GetText("smiled")
```
