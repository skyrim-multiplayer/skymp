# SP 2.8.1 Release Notes


This document includes changes made since SP 2.7.1


WARNING: SP may break your saves. For now, please backup your saves before installing. See https://github.com/skyrim-multiplayer/skymp/issues/796


SP supports both Steam version of Skyrim SE (1.6.640) and legacy 1.5.97.

## Improving the Texts API

Let's use all the power of DirectXTK.
Now you can change the text's size, rotation, font, depth, effects, and origin.

### New added methods
- `setTextSize(textId: number, size: float)` - This function adjusts the size of the text, not the font. It requires a text ID for identification and a floating-point value to set the desired text size.

- `setTextRotation(textId, rotation: float)` - This function rotates the text to a specific angle. It requires a text ID for identification and a floating-point value to specify the degree of rotation.

- `setTextFont(textId, name: string)` - This function modifies the font of the text. It accesses fonts via a relative path "Data/Platform/Fonts/" and the font name is used as the identifier. For example, the font name could be "Tavern".

- `setTextDepth(textId, depth: int)` - This function manages the z-index of the text, thereby determining the display hierarchy on the z-axis. It requires a text ID for identification and an integer value for the z-index.

- `setTextEffect(textId, effect: skyrimPlatform.SpriteEffects)` - This function applies sprite effects to the text. The options include None (0), FlipHorizontally (1), and FlipVertically (2).

- `setTextOrigin(textId, origin: [x,y])` - This function sets the pivot or central point of the text. It requires a text ID for identification and an array that represents the x and y coordinates of the origin.

Should you wish to utilize a custom font, ensure to upload the font in a .sprite version to the "/Data/Platform/Fonts/" directory. When calling the font, reference it by name without the extension.

Please refer to the [Special docs page](../../skyrim_platform/texts.md "Special docs page") for detailed instructions on how to convert any font to .sprite.

```js
// this is a javascript example
sp = skyrimPlatform;

let textId = sp.createText(0, 0, "Hello World!", [1, 1, 1, 1], "Tavern");

sp.on("activate", () => {
  sp.setTextPos(textId, 200, 200);

  sp.setTextString(textId, "Hello World!");

  sp.setTextColor(textId, [1,1,1,1]);

  // new methods usage
  sp.setTextSize(textId, 3);

  sp.setTextRotation(textId, 45);

  sp.setTextFont(textId, "Tavern");

  sp.setTextDepth(textId, 1);

  sp.setTextEffect(textId, sp.SpriteEffects.FlipHorizontally);

  sp.setTextOrigin(textId, [0,0]);
});
```


## Embedded console
Now you can enable cmd and get all the information from the game console in a separate window.

### New SkyrimPlatform.ini settings

- ```Cmd = false``` - This parameter controls whether the console is enabled.

- ```CmdOffsetLeft = 0``` - This parameter controls the console window offset from the left edge of the screen.

- ```CmdOffsetTop = 720``` - This parameter controls the console window offset from the top edge of the screen.

- ```CmdWidth = 1900``` - This parameter controls the width of the console window.

- ```CmdHeight = 317``` - This parameter controls the height of the console window.
> The default values are for full hd

### Why not just use a game console?

- Makes it easier to debug logs.
- If you close the console, the game closes with all background tasks.
- We get the ability to copy text.
- If you open the game in windowed mode, you can play and watch the console.


## Other changes

- Fixed crash in combatState event. See [#1544 (comment)](https://github.com/skyrim-multiplayer/skymp/issues/1544#issuecomment-1628261524)


- Improved script functions call speed. Benchmark: https://github.com/skyrim-multiplayer/skymp/issues/508#issuecomment-1602846718


- Fixed spriggan and TrueHUD related crash
