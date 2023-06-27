# SP 2.1 Release Notes

Changes made since SP 2.0 include the following.

## Add WebPack support

SkyrimPlatform is now able to load plugins built by WebPack. Raw TSC output is still supported and the example plugin didn't migrate. See [skymp5-client](https://github.com/skyrim-multiplayer/skymp/tree/479562345a1f6df4af42217936ccb3e2d3819f78/skymp5-client) for example of use.

Thanks to WebPack support we are now able to use packages from NPM in our SkyrimPlatform plugins. For example, [RxJS](https://rxjs.dev/guide/overview) is proven to work.

Note: packages using timer functions (`setTimeout`, `setInterval`, etc) will not work until we implement these APIs. This also applies to other browser-specific or node-specific APIs.

Also writing plugins in plain JavaScript is now supported for users who do not want bundlers or TypeScript.

```js
// Data/Platform/Plugins/test.js
sp = skyrimPlatform;

sp.printConsole('Hello JS');
```

## Add API to contact back to game from browser

Added `window.skyrimPlatform.sendMessage` to be used on the browser side to talk back to the game. `sendMessage` accepts zero or more JSON-serializable values.

```js
window.skyrimPlatform.sendMessage({ foo: 'bar' });
window.skyrimPlatform.sendMessage(1, 2, 3, 'yay');
window.skyrimPlatform.sendMessage();
```

You can call this function whenever you want: as a button callback, etc.

```html
<input
  type="button"
  value="Click me"
  onclick="window.skyrimPlatform.sendMessage({ foo: 'bar' });"
/>
```

Calls to sendMessage result in a browserMessage event on the SP side. You can handle these events as any others.

```ts
on('browserMessage', (event) => {
  printConsole(JSON.stringify(event.arguments));
});
```

## Add enums for key codes and menus

Added enums for game menus (see [UI Script](https://www.creationkit.com/index.php?title=UI_Script)).

```ts
// Before. Easy way to forget or add excess spaces
Ui.isMenuOpen('ContainerMenu');
Ui.isMenuOpen('InventoryMenu');
Ui.isMenuOpen('Crafting Menu');

// After
Ui.isMenuOpen(Menu.Container);
Ui.isMenuOpen(Menu.Inventory);
Ui.isMenuOpen(Menu.Crafting);
```

And for DirectX scan codes (see [Input Script](https://www.creationkit.com/index.php?title=Input_Script)).

```ts
// Before
Input.isKeyPressed(0x3c); // Sorry?

// After
Input.isKeyPressed(DxScanCode.F2);
```

## Enable local URLs in browser

Now SkyrimPlatform loads `Data/Platform/UI/index.html` if the file exists. It is also possible to load local URLs in runtime.

```ts
// We were able to load only remote URLs. This feature was used in multiplayer but was completely useless for single-player mods.
browser.loadUrl('https://skymp.io');

// Now we can load paths from our local filesystem.
browser.loadUrl('file:///Data/Platform/UI/index.html');
```

## Hot reload strictly in non-game context

Made hot reload to happen strictly in a non-game context.

```ts
import { Game } from 'skyrimPlatform';

// This function call worked in 50% cases of hot reload:
Game.getPlayer();

// Now always throws as expected
```

## Add menuOpen/menuClose events

New events were added: `menuOpen` and `menuClose`.

```ts
on('menuOpen', (e) => {
  printConsole(`The game opens menu: ${e.name}`);
});
```

## Other changes

- Fixed compilation error in SkyrimPlatform plugin example.
- Fixed typo "agressor" in whole project.
- Removed Chromium flag that gives the ability to listen to recording devices via browser-side JavaScript.
- Fixed Nexus antivirus alerts on executables in our archive.
