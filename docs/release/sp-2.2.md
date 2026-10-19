# SP 2.2 Release Notes

This document includes changes made since SP 2.1.

SP updates regularly. This update probably doesn't include ALL patches that have to be made.
There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

Please note that the current SP version only works for the old SE build (before the 11.11.21 update).
To downgrade your Skyrim SE installation use [this patch](https://www.nexusmods.com/skyrimspecialedition/mods/57618).

## Add new enums

- `EquippedItemType`. Types for [Actor.GetEquippedItemType()][EquippedItemType].

  ```ts
  // Before
  player.getEquippedItemType(0) === 1;
  player.getEquippedItemType(0) === 11;

  // After
  player.getEquippedItemType(0) === EquippedItemType.Sword;
  player.getEquippedItemType(0) === EquippedItemType.Torch;
  ```

  Please notice how Battleaxes and Warhammers have both the same value; this is the way vanilla Skyrim works.

- `FormType`. Form types from [`Form.GetType()`][FormType].

  ```ts
  // Before
  cell.getNumRefs(43);
  myBook.getType() === 27;

  // After
  cell.getNumRefs(FormType.NPC);
  myBook.getType() === FormType.Book;
  ```

- `SlotMask`. Armor [slot masks][SlotMask].

  ```ts
  // Before
  armor.getSlotMask() === 0x2000000;
  NiOverride.AddSkinOverrideString(a, true, false, 0x4, 9, 0, diffuseTex, true);

  // After
  armor.getSlotMask() === SlotMask.Jewelry;
  NiOverride.AddSkinOverrideString(
    a,
    true,
    false,
    SlotMask.Body,
    9,
    0,
    diffuseTex,
    true,
  );
  ```

- `WeaponType`. Weapon types from [`Weapon.GetWeaponType()`][WeaponType].

  ```ts
  // Before
  weapon.getWeaponType() === 3;
  weapon.setWeaponType(5);

  // After
  weapon.getWeaponType() === WeaponType.WarAxe;
  weapon.setWeaponType(WeaponType.Greatsword);
  ```

  Please notice how Battleaxes and Warhammers have both the same value; this is the way vanilla Skyrim works.

[EquippedItemType]: https://www.creationkit.com/index.php?title=GetEquippedItemType_-_Actor
[FormType]: https://www.creationkit.com/index.php?title=GetType_-_Form
[SlotMask]: https://www.creationkit.com/index.php?title=Slot_Masks_-_Armor
[WeaponType]: https://www.creationkit.com/index.php?title=GetWeaponType_-_Weapon

## Add consoleMessage event

Called each time the game prints something to the console, including calls to `printConsole`.

Note: The message text can contain any characters, including `'` `"` `\`.
Before sending the text to the browser using "browser.executeJavaScript", it should be escaped.

```typescript
import { browser, on } from 'skyrimPlatform';

const htmlEscapes: Record<string, string> = {
  '"': '\\"',
  "'": "\\'",
  '\\': '\\\\',
  '<': '\\<',
  '>': '\\>',
};

const htmlEscaper = /[&<>"'\\\/]/g;

// On every print to the game console, console.log it to the browser
on('consoleMessage', (e) => {
  const msg = e.message.replace(htmlEscaper, (match) => htmlEscapes[match]);
  browser.executeJavaScript('console.log("' + msg + '")');
});
```

## Add function to disable Ctrl + PrtScn hotkey

This hotkey changes the game speed to be based on framerate rather than real time, which means the game runs normal speed at 30 FPS. Higher than that, the game runs too fast (2x speed at 60 FPS, 4x at 120, etc.) and lower than that, it runs slower. Since not everyone can maintain a constant 60 FPS (framedrops in Riften are common), this hotkey is not allowed.

[Source](https://www.thegamer.com/skyrim-tricks-work-banned/)

Now SP has a method to disable this hotkey:

```ts
import * as sp from 'skyrimPlatform';

sp.disableCtrlPrtScnHotkey();
```

## Other changes

- `PapyrusObject` is now exported from `skyrimPlatform.ts`
