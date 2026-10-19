# Papyrus types added by SkyrimPlatform

SkyrimPlatform currently only adds one type: `TESModPlatform`. Instances of this type do not exist by analogy with `Game`. Its static functions are listed below.

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
- `clearTintMasks` - remove TintMasks for the given Actor or the Player Character if the Actor is not passed.
- `pushTintMask` - add TintMask with def. parameters for the given Actor or the Player Character, if Actor is not passed.
- `pushWornState`, `addItemEx` - add / remove items from def. ExtraData.
- `updateEquipment` - update equipment (unstable).
- `resetContainer` - clear the base container.

# Enums added by Skyrim Platform

Skyrim Platform adds [Typescript enums][Ts-Enum] for both convenience and preventing bugs.

Here's a list of enum values declared in `skyrimPlatform.ts`.

- `DxScanCode`. Key mappings from [Input Script][].

  ```ts
  Input.isKeyPressed(DxScanCode.F2);
  ```

- `EquippedItemType`. Types for [`Actor.GetEquippedItemType()`][EquippedItemType].

  ```ts
  player.getEquippedItemType(0) === EquippedItemType.Sword;
  player.getEquippedItemType(0) === EquippedItemType.Torch;
  ```

  Please notice how Battleaxes and Warhammers have both the same value; this is the way vanilla Skyrim works.

  **_WARNING_**: Don't confuse with `WeaponType` enum. Both appear to be similar, but values actually differ.

- `FormType`. Form types from [`Form.GetType()`][FormType].

  ```ts
  cell.getNumRefs(FormType.NPC);
  myBook.getType() === FormType.Book;
  ```

- `Menu`. Menu names from [UI Script][].

  ```ts
  Ui.isMenuOpen(Menu.Container);
  Ui.isMenuOpen(Menu.Inventory);
  Ui.isMenuOpen(Menu.Crafting);
  ```

- `SlotMask`. Armor [slot masks][SlotMask].

  ```ts
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

  **_WARNING_**: Some slots were left unnamed by Bethesda, but this enum adheres to the [modders' consensus][Biped] on usage for those slots.\
  When making your plugin, be ware **some armor creators may not adhere to that consensus**.

  Some values in this enum are also synonyms, so you can use whichever name you think will best represent what you want to do.

  For example, all these values are the same (`0x10000000`):

  ```ts
  // Consensus dictates slot 0x10000000 may be used for either...
  SlotMask.ArmLeft; // Left arm
  SlotMask.ArmUnder; // Arm undergarment
  SlotMask.ArmSecondary; // Secondary arm
  ```

- `WeaponType`. Weapon types from [`Weapon.GetWeaponType()`][WeaponType].

  ```ts
  weapon.getWeaponType() === WeaponType.WarAxe;
  weapon.setWeaponType(WeaponType.Greatsword);
  ```

  Please notice how Battleaxes and Warhammers have both the same value; this is the way vanilla Skyrim works.

  **_WARNING_**: Don't confuse with `EquippedItemType` enum. Both appear to be similar, but values actually differ.

[Biped]: https://www.creationkit.com/index.php?title=Biped_Object
[EquippedItemType]: https://www.creationkit.com/index.php?title=GetEquippedItemType_-_Actor
[FormType]: https://www.creationkit.com/index.php?title=GetType_-_Form
[Input Script]: https://www.creationkit.com/index.php?title=Input_Script
[SlotMask]: https://www.creationkit.com/index.php?title=Slot_Masks_-_Armor
[Ts-Enum]: https://www.typescriptlang.org/docs/handbook/enums.html
[UI Script]: https://www.creationkit.com/index.php?title=UI_Script
[WeaponType]: https://www.creationkit.com/index.php?title=GetWeaponType_-_Weapon
