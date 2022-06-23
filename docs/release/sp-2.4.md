# SP 2.4.0 Release Notes


This document includes changes made since SP 2.3.0


SP updates regularly. This update probably doesn't include ALL patches that have to be made.

There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

Please note that the current SP version only works for the old SE build (before the 11.11.21 update).

To downgrade your Skyrim SE installation use [this patch](https://www.nexusmods.com/skyrimspecialedition/mods/57618).

## Add `browser.isVisible`/`browser.isFocused` methods

Now we can get an actual state of visible/focused flags instead of remembering it each time we do `setVisible`/`setFocused`.

```typescript
printConsole("isVisible:", browser.isVisible());
printConsole("isFocused:", browser.isFocused());
```


## Support opening URLs in the system default browser

Skyrim Platform already had in-game browser support for creating user interfaces powered by Web technologies.

In this update, we introduce the ability to open URLs in the user's default browser: Chrome, Firefox, Edge, Opera, etc.

Now you are able to create a link or a button or a hotkey or any other trigger that will direct your mod's users to your Nexus, GitHub, or Patreon page. Or even to your Website.

```typescript
win32.loadUrl("https://github.com/skyrim-multiplayer/skymp");
```

Currently, only URLs prefixed with `https://` are allowed.


## Add spellCast event

Now we can catch spellCast events

```typescript
on("spellCast", (event) => {
    printConsole(event.caster.getFormID());
    printConsole(event.spell.getName());
})
```

Event object contains `caster: ObjectReference` and `spell: Spell` fields.


## Support array return values in scripting functions

Now we can call functions that return arrays instead of getting exceptions each time we use such scripting functions.

```typescript
Utility.createStringArray(3, "teststring");
// ["teststring", "teststring", "teststring"]
Utility.createBoolArray(3, true);
// [true, true, true]

const spell = player.getNthSpell(0);
browser.executeJavascript(`console.log(${JSON.stringify(s?.getEffectDurations())})`);
// prints numbers array to browser console [5, 5, 5];
const array = spell?.getMagicEffects();
// return array of PapyrusObjects
array?.forEach(element => {
    printConsole(MagicEffect.from(element)?.getFormID());
    // create MagicEffect from PapyrusObject with static .from
    // print FormID for all MagicEffects from 1st spell
})
```

Note that currently we always get `PapyrusObject[]` in case of object array return.
So we need to use the static method `from` for elements to use class methods.


## Other changes

- Removed non-existing `writeScript` method from `skyrimPlatform.ts`. `writePlugin` can be used instead.
