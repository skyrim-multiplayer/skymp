# Skyrim Platform

Skyrim Platform is a modding tool for Skyrim allowing writing scripts with JavaScript/TypeScript. One of the mods built on Skyrim Platform is skymp client. Yes, client of Skyrim Multiplayer is technically a mod for Skyrim Special Edition implemented using Skyrim Platform.

### Papyrus types from the original game

- All types in SkyrimPlatform have the same name as in Papyrus, for example: `Game`, `Actor`, `Form`, `Spell`, `Perk`, etc.

- To use types from Papyrus, including calling methods and static functions that they have, they need to be imported:

  ```typescript
  import { Game, Actor } from "skyrimPlatform";
  ```

### Native functions

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

### Native functions from SKSE plugins

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

### Form

- Form (`Form`) is inherited by most game types, which have methods such as `Actor`, `Weapon`, etc.
- Each form has an ID, which is a 32-bit unsigned number (`uint32_t`). In SkyrimPlatform, represented by the type `number`.
- If you need to find a form by its ID, use `Game.getFormEx`. Note that it is `Game.getFormEx`, not `Game.getForm`. The latter always returns `null` for IDs above 0x80000000 (the behavior of the original game).
- You can get the form ID using the `getFormID` method. It is guaranteed that `Game.getFormEx` will find the form by the ID returned by this method if the form was not destroyed by the game.

### Safe use of objects

- After you get the object, you need to make sure that it is not `null`:
  ```typescript
  let actor = Game.findClosestActor(x, y, z, radius);
  if (actor) {
    let isInCombat = actor.isInCombat();
  }
  ```
  Or
  ```typescript
  let actor = Game.findClosestActor(x, y, z, radius);
  if (!actor) return;
  let isInCombat = actor.isInCombat();
  ```
- It is guaranteed that `Game.getPlayer` never returns `null`.
- Use the `strict` option in `tsconfig.json` to enable/disable compiler null checks and other correctness checks. Learn more here: https://www.typescriptlang.org/tsconfig#strict

### Unhandled exceptions

- Unhandled JS exceptions will be logged to the console along with the call stack.
- Raw Promise rejections are also output to the console.
- Do not release plugins that have known bugs that are not handled. SkyrimPlatform performance is not guaranteed with unhandled exceptions.

### Object comparison

- To compare objects in SkyrimPlatform, you need to compare their IDs:
  ```typescript
  if (object1.getFormId() === object2.getFormId()) {
    // ...
  }
  ```

### Casting objects to string

- Types ported from Papyrus have limited support for a number of operations normal for regular JS objects such as `toString`, `toJSON`.
  ```typescript
  Game.getPlayer().ToString(); // '[object Actor]'
  JSON.stringify(Game.getPlayer()); // `{}`
  ```

### Cast

- If you have a `Form` object that is a weapon, and you need a `Weapon` object, you can use casting:
  ```typescript
  let sword = Game.getFormEx(swordId); // Get Form
  let weapon = Weapon.from(sword); // Cast to Weapon
  ```
- If you specify an ID for a form that is not actually a weapon, the `weapon` variable will be `null`.
- Passing `null` to the function for casting types as an argument will not throw an exception, but will return `null`:
  ```typescript
  ObjectReference.from(null); // null
  ```
- An attempt to cast to a type that has no instances or is incompatible in the inheritance hierarchy will also return `null`:
  ```typescript
  Game.from(Game.getPlayer()); // null
  Spell.from(Game.getPlayer()); // null
  ```
- You can also use typecasting to get an object of the base type, including `Form`:
  ```typescript
  let refr = ObjectReference.from(Game.getPlayer());
  let form = Form.from(refr);
  ```
- Casting an object to its own type will return the original object:
  ```typescript
  let actor = Actor.from(Game.getPlayer());
  ```

### Papyrus types added by SkyrimPlatform

- SkyrimPlatform currently only adds one type: `TESModPlatform`. Instances of this type do not exist by analogy with `Game`. Its static functions are listed below.
- `moveRefrToPosition` - teleports the object to the specified location and position.
- `setWeaponDrawnMode` - forces the actor to always keep the weapon drawn / removed.
- `getNthVtableElement` - gets the offset of the function from the virtual table (for reverse engineering).
- `getSkinColor` - gets the skin color of the ActorBase.
- `createNpc` - creates a new form of type ActorBase.
- `setNpcSex` - changes the gender of the ActorBase.
- `setNpcRace` - changes the race of the ActorBase.
- `setNpcSkinColor` - changes the skin color of the ActorBase.
- `setNpcHairColor` - changes the hair color of the ActorBase.
- `resizeHeadpartsArray` - resizes the array of head parts ActorBase.
- `resizeTintsArray` - resizes the main character's TintMasks array.
- `setFormIdUnsafe` - changes the form ID. Unsafe, use at your own risk.

* `clearTintMasks` - remove TintMasks for the given Actor or the Player Character if the Actor is not passed.
* `pushTintMask` - add TintMask with def. parameters for the given Actor or the Player Character, if Actor is not passed.
* `pushWornState`, `addItemEx` - add / remove items from def. ExtraData.
* `updateEquipment` - update equipment (unstable).
* `resetContainer` - clear the base container.

### Asynchronous

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

### Events

- At the moment, SkyrimPlatform has the ability to subscribe to your own events: `update` and` tick`.

- `update` is an event that is called once for every frame in the game (60 times per second at 60 FPS) after you've loaded a save or started a new game.

  ```typescript
  import { on } from "skyrimPlatform";
  on("update", () => {
    // At this stage, the methods of all imported
    // types are already available.
  });
  ```

- `tick` is an event that is called once for every frame in the game immediately after the game starts.
  ```typescript
  import { on } from "skyrimPlatform";
  on("tick", () => {
    // No access to game methods here.
  });
  ```
- And also for game events such as `effectStart`, `effectFinish`, `magicEffectApply`, `equip`, `unequip`, `hit`, `containerChanged`, `deathStart`, `deathEnd`, `loadGame`,  `combatState`, `reset`, `scriptInit`, `trackedStats`, `uniqueIdChange`, `switchRaceComplete`, `cellFullyLoaded`, `grabRelease`, `lockChanged`, `moveAttachDetach`, `objectLoaded`, `waitStop`, `activate` ...

- With `on`, you can subscribe to the event forever.
  ```typescript
  import { on } from "skyrimPlatform";
  on("equip", (event) => {
    printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
    printConsole(`object: ${event.baseObj.getName()}`);
  });
  ```
- Using `once`, you can add a handler that will be called once the next time the event is fired.
  ```typescript
  import { once } from "skyrimPlatform";
  once("equip", (event) => {
    printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
    printConsole(`object: ${event.baseObj.getName()}`);
  });
  ```
- The variable `even` always contains variables related to the event to which you are subscribed.

### Hooks

- Hooks allow you to intercept the start and end of some functions of the game engine.
- Currently supported hooks: `sendAnimationEvent`
  ```typescript
  import { hooks, printConsole } from "skyrimPlatform"
  hooks.sendAnimationEvent.add({
  	enter(ctx) {
  		printConsole(ctx.animEventName);
  	},
  	leave(ctx) {
  		if (ctx.animationSucceeded) printConsole(ctx.selfId);
  	};
  });
  ```
- `enter` is called before starting the function. `ctx` contains the arguments passed to the function and also` storage` (see below).
- `leave` is called before the function ends. `ctx` contains the return value of the function, in addition to what was after the completion of` enter`.
- `ctx` is the same object for calls to` enter` and `leave`.
- `ctx.storage` is used to store data between calls to` enter` and `leave`.
- Script functions are not available inside the `enter` and` leave` handlers.

### Custom SkyrimPlatform Methods and Properties

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

### Changing game console commands

- SkyrimPlatform allows you to change the implementation of any game console command, for such a modification you need to get the console command object by passing the command name to the `findConsoleCommand (commandName)` method, short or long.

  ```typescript
  let getAV = findConsoleCommand("GetActorValueInfo");
  let getAV = findConsoleCommand("GetAVInfo");
  ```

- Having received such an object, you can change the short (`shortName`) or long (`longName`) command name, as well as the number of accepted arguments (`numArgs`) and the function (`execute`) that will be executed when this console command is called via game console.

  ```typescript
  getAV.longName = "printArg";
  getAV.shortName = "";
  getAV.execute = (refrId: number, arg: string) => {
    printConsole(arg);
    return false;
  };
  ```

- The return value of your new implementation indicates whether the original function of this command will be executed.
- The first argument is the FormId of the object on which the console command is called, or 0 if it is absent.
- The rest of the parameters will be the arguments with which the console command was called, of type `string` or `number`.
- Since game functions are not available in this context, you must register an `update` event handler with` once` if you want to call a game function when you invoke a console command:

```typescript
getAV.longName = "ShowMessageBox";
getAV.shortName = "";

getAV.execute = (refrId: number, arg: string) => {
  once("update", () => {
    Debug.messageBox(arg);
  });
  return false;
};
```

### HTTP requests

SkyrimPlatform provides limited support for HTTP/HTTPS requests.
At the moment only `get` and `post` are available.

```typescript
import { HttpClient } from "skyrimPlatform";
let url = "https://canhazip.com:443"; // URL may contain port or not
let http = new HttpClient(url);
http.get("/").then((response) => printConsole(response.body));
```

- In case the request fails, `response.body` will be empty.

### Browser

#### Basics

Create `Data/Platform/UI/index.html` with contents below to test:
```html
<font color="white"><h1>Hello SP</h1></font>
```

SkyrimPlatform loads `Data/Platform/UI/index.html` if the file exists. It is also possible to load URLs in runtime.

```typescript
import { browser } from "skyrimPlatform";

// Enable/disable browser visibility
browser.setVisible(true);

// Open cursor and redirect mouse and keyboard events to the browser
browser.setFocused(true);

// Load a specified URL. The current implementation loads URLs only after the user moves the mouse, except the default URL.
browser.loadUrl("file:///Data/Platform/UI/index.html");         // Default one
browser.loadUrl("");                                            // Same effect for empty URL
browser.loadUrl("file:///Data/Platform/UI/another-file.html");  // Load another page from Data
browser.loadUrl("https://google.com");                          // Open websites
browser.loadUrl("http://localhost:9000");                       // Open remote dev tools. You better open them in normal browser
browser.loadUrl("http://localhost:1234");                       // Your favorite dev server in watch mode

// Execute JavaScript code in browser context
browser.executeJavaScript("console.log('Hello CEF')");

// SkyrimPlatform generates a unique token every game start
// In browser context, it appears in `window.spBrowserToken` after some time from page load moment
const str = browser.getToken();
```

#### Talking back to the game

Use `window.skyrimPlatform.sendMessage` on the browser side to talk back to the game. `sendMessage` accepts zero or more JSON-serializable values.
```js
window.skyrimPlatform.sendMessage({ foo: 'bar' });
window.skyrimPlatform.sendMessage(1, 2, 3, "yay");
window.skyrimPlatform.sendMessage();
```

You can call this function whenever you want: as a button callback, etc.
```html
<input type="button" value="Click me" onclick="window.skyrimPlatform.sendMessage({ foo: 'bar' });">
```

Calls to `sendMessage` result in a `browserMessage` event on the SP side. You can handle these events as any others.
```ts
on("browserMessage", (event) => {
  printConsole(JSON.stringify(event.arguments));
});
```

"Ping-pong" example: SP context communicates with browser context via `executeJavaScript`, and the browser context communicates back with `window.skyrimPlatform.sendMessage`.
```ts
once("tick", () => {
    browser.executeJavaScript("window.skyrimPlatform.sendMessage('yay')");
});

on("browserMessage", (event) => {
    printConsole(JSON.stringify(event.arguments));
    browser.executeJavaScript("window.skyrimPlatform.sendMessage('yay')");
});
```

### Hot Reload

- Hot Reload for SkyrimPlatform plugins is supported. Changing the contents of `Data / Platform / Plugins` will reload all plugins without restarting the game.
- For using all these features, including Ctrl+S hot reload, take our example plugin as a base https://github.com/skyrim-multiplayer/skymp/tree/main/skyrim-platform/tools/plugin-example. Plugin example is also present in the archive uploaded to Nexus Mods (Data/Platform/plugin-example).
- When reloading plugins, the added event and hook handlers are removed, asynchronous operations are interrupted and all variables are reset, except for `storage` and its properties.

### DumpFunctions

- SkyrimPlatform has built-in functionality that allows you to output information about game functions to the file `Data / Platform / Output / DumpFunctions.txt` (key combination 9 + O + L). The game pauses for a few seconds while DumpFunctions is running.
