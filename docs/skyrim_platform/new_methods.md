# Custom Skyrim Platform Methods and Properties

There are methods such as `printConsole ()` that can be called immediately after import. They do not belong to any of the game types.

## Table of contents

  - [browser](#browser)
  - [on/once](#ononce)
  - [printConsole](#printconsole)
  - [settings](#settings)
  - [writeLogs](#writelogs)
  - [Other methods and properties](#other-methods-and-properties)


## printConsole

  `printConsole (... arguments: unknown[]): void`

Output to the game console, opened by the `~` key.

All console messages will get the string `[Script]` appended.

  ```typescript
  import { printConsole, Game, once } from "skyrimPlatform";

  once("update", () => {
    printConsole(`player id = ${Game.getPlayer().getFormID()}`);
  });
  ```

## writeLogs

  `writeLogs(pluginName: string, ...arguments: unknown[]): void`

Write to a log file.\
This function will create a file named `Data/Platform/Plugins/pluginName-logs.txt`.

Notice how this function doesn't enforce any particular formatting and doesn't automatically write things like timestamps.\
This gives you great freedom on what info you can include in your log and how you want to show it.

For example, you can use `Date().toLocaleString()` if you want to put a timestamp in the locale of the computer the game is currently running in:

```typescript
import { writeLogs } from "skyrimPlatform";

const t = new Date().toLocaleString();
writeLogs("your-mod", `${t}: some message`);
```

***WARNING***: This file will be overwritten each time the game is closed.\
If you want to have many logs, you will need to explicitly handle log names.

## settings

`settings`

An object that provides access to plugin settings:

```typescript
import { settings, printConsole } from "skyrimPlatform";

let option = settings["plugin-name"]["my-option"];
printConsole(option);
```

The plugin settings file is named `plugin-settings.txt` and should be located in the` Data/Platform/Plugins` folder.\
File format JSON, extension `.txt` for the convenience of users.

Changing a settings file contents will fire [hot reloading][Features].

## on/once
  `on (eventName: string, callback: any): void`

  `once (eventName: string, callback: any): void`

[Subscribe][NewEvents] to an event named `eventName`.

## browser

An object providing [access to the Chromium Embedded Framework][Browser].

## Other methods and properties

More info and samples for these will be added later:

- `worldPointToScreenPoint` - convert an array of points in the game world to an array of points on the user's screen. The dot on the screen is indicated by 3 numbers from -1 to 1.
- `callNative (className: string, functionName: string, self ?: object, ... args: any): any` - call a function from the original game by name.
- `getJsMemoryUsage (): number` - get the amount of RAM used by the embedded JS engine, in bytes.
- `storage` - an object used to save data between reloading scripts. See the [Plugin Initialization][PluginInit] recipe for an example of how to use it.
- `getExtraContainerChanges` - get ExtraContainerChanges of the given ObjectReference...
- `getContainer` - get all the items of the base container.
- `disableCtrlPrtScnHotkey` - disable hotkey that changes the game speed to be based on framerate rather than real time.

[Browser]: browser.md
[Features]: features.md
[NewEvents]: new_events.md
[PluginInit]: cookbook.md#plugin-initialization
