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
