# The Skyrim Platform Cook Book

Here you will find some easy and tasty recipes for commonly used tasks and commonly asked questions.

## Table of contents

- [Saving variables](#saving-variables)
- [Plugin initialization](#plugin-initialization)
- [Getting rid of cloaks](#getting-rid-of-cloaks)

## Saving variables

One of the many things that sets Skyrim Platform (SP) apart from Papyrus is that SP doesn't bake variable values into saves.

This isn't good or bad. It just _is_.\
But depending on what you want to do, it may be convenient or inconvenient.

In this recipe we will save variable values, so they can be restored when the player (re)loads a saved game.

### Ingredients

- [JContainers][]
- [JContainers Typescript definitions][TsTypings]

You may use [PapyrusUtil][] as well, but we will be using JContainers for this recipe because it's deemed to be harder to use than PapyrusUtil.

### Preparation

1. Follow instructions on [how to install][Papyrus2Ts] JContainers Typescript definitions.

2. Import `JDB` from JContainers.

```ts
import * as JDB from 'JContainers/JDB';
```

3. Create a `key` name for your mod.

```ts
const key = '.my-unique-mod-name';
```

**_WARNING_**: Your key name must ALWAYS start with `.` otherwise this recipe won't work.

It's recommended it doesn't contain spaces.

4. Save values to your liking.

```ts
JDB.solveIntSetter(key + '.someInt', someInt, true);
JDB.solveFltSetter(key + '.someFlt', someFlt, true);
JDB.solveStrSetter(key + '.someStr', someStr, true);
```

5. Restore values to your liking.

```ts
someInt = JDB.solveInt(key + '.someInt');
someFlt = JDB.solveFlt(key + '.someFlt');
someStr = JDB.solveStr(key + '.someStr');
```

## Plugin initialization

Plugin variables need to be initialized when creating a new game and when loading a saved game, but there's no event sent when a new game has been created.

This recipe let us initialize a plugin when:

- A new game is started.
- It's first installed.
- A game has been reloaded.
- Skyrim Platform's [hot reloading][HotReload] feature runs.

### Ingredients

Same ingredients as [Saving variables](#saving-variables).\
This recipe is an extension of the techniques learned there.

### Preparation

1. Create functions to save/load a `boolean`. These will check if our plugin was already initialized.\
   Imagine we are asking if our plugin has already gone through an [`OnInit`][OnInit] event.

```ts
const initK = '.my-plugin.init';

const MarkInitialized = () => JDB.solveBoolSetter(initK, true, true);
const WasInitialized = () => JDB.solveBool(initK, false);
```

2. Add [event hooks][NewEvents] to both `loadGame` and `update`.\
   \
   Using `once("update")` means our code will run once as soon as the game has started or hot reloading happened.

```ts
on('loadGame', () => {});

// IMPORTANT: we are using ONCE instead of on.
once('update', () => {});
```

3. Add a `boolean` variable to check if your plugin has already been initialized by `loadGame`.\
   This will avoid initialization to be done twice when loading an existing save, but allows it to happen when hot reloading.

```ts
let allowInit = false;

on('loadGame', () => {
  allowInit = true;
});

once('update', () => {
  if (allowInit) {
    InitPlugin();
  }
});
```

4. Check if your plugin has ever been initialized.\
   \
   Usually, `loadGame` will let you do initializons when your plugin is installed mid game, but this step lets you do them **when creating a new game**.\
   \
   Here we need to use the [`storage`][NewMethods] Map so the value of `allowInit` is remembered between hot reloadings.

```ts
let allowInit = storage['my-plugin-init'] as boolean | false;

on('loadGame', () => {
  // Initialize when installed mid game and when loading a game
  // because this is always needed anyway.
  InitPlugin();
  allowInit = true;
  storage['my-plugin-init'] = true;
});

once('update', () => {
  // Has this plugin ever been initialized?
  // OnInit facsimile.
  if (allowInit || !WasInitialized()) {
    InitPlugin();
  }
});
```

5. Initialize your plugin to your needs.\
   \
   Using `storage` is not really necessary if we save to disk `pluginVar1` each time we set a new value it (a must if you want your plugin values to be persistent, anyway), but we will use it here to demonstrate how it is used.

```ts
// Initialize with the value it had before hot reloading or default 0
let pluginVar1 = storage['my-plugin-var1'] as number | 0;

function InitPlugin() {
  const key = '.my-plugin.var1';

  // Initialize with the value it had before reloading the game or default 0
  pluginVar1 = JDB.solveFlt(key, 0);

  // Save values inmediately to both disk and storage, so they don't get lost.
  JDB.solveFltSetter(key, pluginVar1, true);
  storage['my-plugin-var1'] = pluginVar1;

  // Let's suppose this was OnInit.
  MarkInitialized();
}
```

Here's the full code:

```ts
import * as JDB from 'JContainers/JDB';
import { on, once } from 'skyrimPlatform';

const initK = '.my-plugin.init';

const MarkInitialized = () => JDB.solveBoolSetter(initK, true, true);
const WasInitialized = () => JDB.solveBool(initK, false);

export function main() {
  let allowInit = storage['my-plugin-init'] as boolean | false;

  on('loadGame', () => {
    // Initialize when installed mid game and when loading a game
    // because this is always needed anyway.
    InitPlugin();
    allowInit = true;
    storage['my-plugin-init'] = true;
  });

  // IMPORTANT: we are using ONCE instead of on.
  once('update', () => {
    // Has this plugin ever been initialized?
    // OnInit facsimile.
    if (allowInit || !WasInitialized()) {
      InitPlugin();
    }
  });

  // Initialize with the value it had before hot reloading or default 0
  let pluginVar1 = storage['my-plugin-var1'] as number | 0;

  function InitPlugin() {
    const key = '.my-plugin.var1';

    // Initialize with the value it had before reloading the game or default 0
    pluginVar1 = JDB.solveFlt(key, 0);

    // Save values inmediately to both disk and storage, so they don't get lost.
    JDB.solveFltSetter(key, pluginVar1, true);
    storage['my-plugin-var1'] = pluginVar1;

    // Let's suppose this was OnInit.
    MarkInitialized();
  }
}
```

## Getting rid of cloaks

Cloaks are [infamous for being inefficient][Cloaks], but thanks to the [new events][NewEvents] defined in SP, they are becoming increasingly unneeded.

The basic need that made cloaks necessary in the past is this:

> Some code needs to be executed on some `Actor`.

If your plugin still needs to use this idea, but none of the new events suits you, then you can use a more efficient method; a method we will use for this recipe.

### Ingredients

- [SPID][]

### Preparation

1. Create a new [Magic Effect][MgFx] in the CK.
   - Effect Archetype: Script
   - Casting Type: Constant
   - Delivery: Self
   - Fill other properties according to your needs.

Lets suppose this magic effect has a relative FormId of `0x800`.

2. Create a new [spell][Spell].

   - Type: Ability
   - Casting Type: Constant effect.
   - Delivery: Self.
   - Effects: the magic effect you created in the previous step.
   - Fill other properties according to your needs.

3. Distribute that spell according to SPID instructions.
4. Catch the magic effect being applied by using the [`effectStart`][EffectStart] event.

```typescript
import { on } from 'skyrimPlatform';

on('effectStart', (event) => {
  const fx = Game.getFormFromFile(0x800, 'my-mod.esp');
  if (fx?.getFormID() !== event.effect.getFormID()) return;

  DoSomething(event.target);
});
```

5. (Optional) you can use the [`effectFinish`][EffectFinish] event if you need to do some cleaning.

```typescript
on('effectFinish', (event) => {
  const fx = Game.getFormFromFile(0x800, 'my-mod.esp');
  if (fx?.getFormID() !== event.effect.getFormID()) return;

  DoSomeCleaning(event.target);
});
```

[Cloaks]: https://www.reddit.com/r/skyrimmods/comments/4fgf3n/looking_for_deep_explanation_of_the_cloaking/
[EffectFinish]: new_events.md#effectfinish
[EffectStart]: new_events.md#effectstart
[HotReload]: features.md#hot-reload
[JContainers]: https://www.nexusmods.com/skyrimspecialedition/mods/16495
[MgFx]: https://www.creationkit.com/index.php?title=Magic_Effect
[NewEvents]: new_events.md
[OnInit]: https://www.creationkit.com/index.php?title=OnInit
[Papyrus2Ts]: https://www.nexusmods.com/skyrimspecialedition/mods/56916
[PapyrusUtil]: https://www.nexusmods.com/skyrimspecialedition/mods/13048
[Spell]: https://www.creationkit.com/index.php?title=Spell
[SPID]: https://www.nexusmods.com/skyrimspecialedition/mods/36869
[TsTypings]: https://www.nexusmods.com/skyrimspecialedition/mods/56916?tab=files
[NewMethods]: new_methods.md
