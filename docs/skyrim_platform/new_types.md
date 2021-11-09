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
  Ui.isMenuOpen(Menu.Container);
  Ui.isMenuOpen(Menu.Inventory);
  Ui.isMenuOpen(Menu.Crafting);
  ```

- `Menu`. Menu names from [UI Script][].
  ```ts
  Input.isKeyPressed(DxScanCode.F2);
  ```


[UI Script]: https://www.creationkit.com/index.php?title=UI_Script
[Input Script]: https://www.creationkit.com/index.php?title=Input_Script
[Ts-Enum]: https://www.typescriptlang.org/docs/handbook/enums.html
