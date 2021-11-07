## Add enums for key codes and menus

Added enums for game menus (see [UI Script](https://www.creationkit.com/index.php?title=UI_Script)).

```ts
// Before. Easy way to forget or add excess spaces
Ui.isMenuOpen("ContainerMenu");
Ui.isMenuOpen("InventoryMenu");
Ui.isMenuOpen("Crafting Menu");

// After
Ui.isMenuOpen(Menu.Container);
Ui.isMenuOpen(Menu.Inventory);
Ui.isMenuOpen(Menu.Crafting);
```

And for DirectX scan codes (see [Input Script](https://www.creationkit.com/index.php?title=Input_Script)).

```ts
// Before
Input.isKeyPressed(0x3C); // Sorry?

// After
Input.isKeyPressed(DxScanCode.F2);
```
