# Custom SkyrimPlatform Methods and Properties

- There are methods such as `printConsole ()` that can be called immediately after import. They do not belong to any of the game types.
- `printConsole (... arguments: any []): void` - output to the game console, opened by the `~` key.

  ```typescript
  import { printConsole, Game } from "skyrimPlatform";
  on("update", () => {
    printConsole(`player id = ${Game.getPlayer().getFormID()}`);
  });
  ```

- `worldPointToScreenPoint` - convert an array of points in the game world to an array of points on the user's screen. The dot on the screen is indicated by 3 numbers from -1 to 1.
- `on (eventName: string, callback: any): void` - subscribe to an event named` eventName`.
- `callNative (className: string, functionName: string, self ?: object, ... args: any): any` - call a function from the original game by name.
- `getJsMemoryUsage (): number` - get the amount of RAM used by the embedded JS engine, in bytes.
- `storage` - an object used to save data between reloading scripts.
- `browser` is an object providing access to the Chromium Embedded Framework.
- `getExtraContainerChanges` - get ExtraContainerChanges of the given ObjectReference...

* `getContainer` - get all the items of the base container.
* `settings` - an object that provides access to plugin settings:

  ```typescript
  import { settings, printConsole } from "skyrimPlatform";
  let option = settings["plugin-name"]["my-option"];
  printConsole(option);
  ```

  The plugin settings file is named `plugin-settings.txt` and should be located in the` Data / Platform / Plugins` folder.
  File format - JSON, extension `.txt` - for the convenience of users.
