# The Skyrim Platform Cook Book

Here you will find some easy and tasty recipes for commonly used tasks and commonly asked questions.

## Table of contents

  - [Saving variables](#saving-variables)
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
  import * as JDB from "JContainers/JDB";
  ```

3. Create a `key` name for your mod.

  ```ts
  const key = ".my-unique-mod-name";
  ```

  ***WARNING***: Your key name must ALWAYS start with `.` otherwise this recipe won't work.

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
  import { on } from "skyrimPlatform";

  on("effectStart", (event) => {
    const fx = Game.getFormFromFile(0x800, "my-mod.esp");
    if (fx?.getFormID() !== event.effect.getFormID()) return;

    DoSomething(event.target);
  })
  ```

5. (Optional) you can use the [`effectFinish`][EffectFinish] event if you need to do some cleaning.

  ```typescript
  on("effectFinish", (event) => {
    const fx = Game.getFormFromFile(0x800, "my-mod.esp");
    if (fx?.getFormID() !== event.effect.getFormID()) return;

    DoSomeCleaning(event.target);
  })
  ```

[Cloaks]: https://www.reddit.com/r/skyrimmods/comments/4fgf3n/looking_for_deep_explanation_of_the_cloaking/
[JContainers]: https://www.nexusmods.com/skyrimspecialedition/mods/16495
[MgFx]: https://www.creationkit.com/index.php?title=Magic_Effect
[NewEvents]: new_events.md
[Papyrus2Ts]: https://www.nexusmods.com/skyrimspecialedition/mods/56916
[PapyrusUtil]: https://www.nexusmods.com/skyrimspecialedition/mods/13048
[SPID]: https://www.nexusmods.com/skyrimspecialedition/mods/36869
[TsTypings]: https://www.nexusmods.com/skyrimspecialedition/mods/56916?tab=files
[Spell]: https://www.creationkit.com/index.php?title=Spell
[EffectStart]: new_events.md#effectstart
[EffectFinish]: new_events.md#effectfinish
