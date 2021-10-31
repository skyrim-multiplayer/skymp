# Native functions

- Most types have a list of native functions, they are divided into static functions (`Native Global` in Papyrus) and methods (`Native`).

- Static functions are called on the type:

  ```typescript
  let sunX = Game.getSunPositionX();
  let pl = Game.getPlayer();
  Game.forceFirstPerson();
  ```

- Methods are called on the object:

  ```typescript
  let isPlayerInCombat = pl.isInCombat();
  ```

- A list of original game types with documentation can be found here: https://www.creationkit.com/index.php?title=Category:Script_Objects
- Calling functions from the original game is only available inside the `update` event handler (see below). If you try to do this in a different context, an exception will be thrown.

# Native functions from SKSE plugins

- You are able to call Papyrus functions added by SKSE plugins. This example illustrates how to use PapyrusUtil in a SkyrimPlatform plugin:
  ```typescript
  import * as sp from "skyrimPlatform";

  sp.on("update", () => {
    const filePath = "data/platform/plugins/plugin-example.js";

    // SkyrimPlatform doesn't contain PapyrusUtil typings so we cast to any to be able to call all functions.
    // Here we use 'MiscUtil.ReadFromFile' native. Of course, you can use any other function.
    const str = (sp as any).MiscUtil.readFromFile(filePath);

    sp.printConsole("Read", str.length, "characters");
  });
  ```

# Asynchronous

- Some game functions take time and happen in the background. Such functions in SkyrimPlatform return `Promise`:
  ```typescript
  Game.getPlayer()
    .SetPosition(0, 0, 0)
    .then(() => {
      printConsole("Teleported to the center of the world");
    });
  ```
  ```typescript
  Utility.wait(1).then(() => printConsole("1 second passed"));
  ```
- When called asynchronously, execution continues immediately:
  ```typescript
  Utility.wait(1);
  printConsole(`Will be displayed immediately, not after a second`);
  printConsole(`Should have used then`);
  ```
- You can use `async`/`await` to make the code look synchronous:
  ```typescript
  let f = async () => {
    await Utility.wait(1);
    printConsole("1 second passed");
  };
  ```
