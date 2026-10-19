# Native functions

Most types have a list of native functions, they are divided into static functions (`Native Global` in Papyrus) and methods (`Native`).

Static functions are called on the type:

```typescript
let sunX = Game.getSunPositionX();
let pl = Game.getPlayer();
Game.forceFirstPerson();
```

Methods are called on the object:

```typescript
let isPlayerInCombat = pl.isInCombat();
```

A list of original game types with documentation can be found here: https://www.creationkit.com/index.php?title=Category:Script_Objects

**_WARNING_**: Papyrus types and methods are only available inside [new events][NewEvents] introduced by Skyrim Platform (except `tick`) or `sendPapyrusEvent` hook (see [hooks][Hooks]).

An exception will be thrown if you try to use them outside any of those contexts.

# Native functions from SKSE plugins

You are able to call Papyrus functions added by SKSE plugins.

This example illustrates how to use PapyrusUtil in a Skyrim Platform plugin:

```typescript
import * as sp from 'skyrimPlatform';

sp.once('update', () => {
  const filePath = 'data/platform/plugins/plugin-example.js';

  // SkyrimPlatform doesn't contain PapyrusUtil typings so we cast to any to be able to call all functions.
  // Here we use 'MiscUtil.ReadFromFile' native. Of course, you can use any other function.
  const str = (sp as any).MiscUtil.readFromFile(filePath);

  sp.printConsole('Read', str.length, 'characters');
});
```

<!-- TODO: Delete when this functionality is done at engine level -->

Another option is to use [converted library definitions][TsDefs] or make them yourself by using [Papyrus-2-Typescript][Papyrus2Ts]; then you can use native SKSE functions the same way you would use them in Papyrus:

```typescript
import * as MiscUtil from 'PapyrusUtil/MiscUtil';
import { once, printConsole } from 'skyrimPlatform';

once('update', () => {
  const filePath = 'data/platform/plugins/plugin-example.js';

  // PapyrusUtil typings are defined inside MiscUtil.ts
  const str = MiscUtil.readFromFile(filePath);

  printConsole('Read', str.length, 'characters');
});
```

# Asynchronous

Some game functions take time and happen in the background. Such functions in SkyrimPlatform return `Promise`:

```typescript
Game.getPlayer()
  .SetPosition(0, 0, 0)
  .then(() => {
    printConsole('Teleported to the center of the world');
  });
```

```typescript
Utility.wait(1).then(() => printConsole('1 second passed'));
```

When called asynchronously, execution continues immediately:

```typescript
Utility.wait(1);
printConsole(`Will be displayed immediately, not after a second`);
printConsole(`Should have used then`);
```

You can use `async`/`await` to make the code look synchronous:

```typescript
let f = async () => {
  await Utility.wait(1);
  printConsole('1 second passed');
};
```

**_WARNING_**: When using asynchronous functions inside [new events][NewEvents], `Form` references may be already lost when the time to do something with them has come.

For those cases, it's possible to save the `FormId` and retrieve the `Form` again with [`Game.getFormEx`][GetFormEx].

```typescript
on('unequip', (event) => {
  // Define an async function.
  const f = async () => {
    const id = event.actor.getFormID();
    await Utility.wait(0.1);
    // event.actor is lost at this point. We need to get it again.
    const a = Game.getFormEx(id);
    printConsole(`${a?.getName()} unequipped something.`);
  };

  // Execute previosly defined async function
  f();
});
```

[GetFormEx]: papyrus.md#form
[Hooks]: events.md
[NewEvents]: new_events.md
[Papyrus2Ts]: https://www.nexusmods.com/skyrimspecialedition/mods/56916
[TsDefs]: https://github.com/CarlosLeyvaAyala/Papyrus-2-Typescript/tree/main/conversions
