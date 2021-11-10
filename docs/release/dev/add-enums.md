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
[WeaponType]: https://www.creationkit.com/index.php?title=GetWeaponType_-_Weapon
