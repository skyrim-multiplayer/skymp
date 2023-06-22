# Improving the texts API
## Let's use all the power of DirectXTK

Now you can change the size, rotation, font, depth, effects and origin of the text.

### New added methods
- `setTextSize(textId: number, size: float)` - set text (not font) size.

- `setTextRotation(textId, rotation: float)` - set rotation.

- `setTextFont(textId, name: string)` - set font from relative path "Data/Platform/Fonts/", by name — "Tavern".

- `setTextDepth(textId, depth: int)` - set z-index of the text.

- `setTextEffect(textId, effect: skyrimPlatform.SpriteEffects)` - set sprite effect [None = 0, FlipHorizontally = 1, FlipVertically = 2]

- `setTextOrigin(textId, origin: [x,y])` - set text pivot (center point).

>To use your own font, upload .sprite version to the "/Data/Platform/Fonts/" folder and call by name WITHOUT extension

> How to convert any font to .sprite, you can find in the [Special docs page](https://github.com/VitaliyMubarakov/skymp/blob/cb65bfcd742a7bf8963ca35d30ca8c2d9dd3311b/docs/skyrim_platform/texts.md "Всплывающая подсказка")

### JS Example
```js
sp = skyrimPlatform;

let textId = sp.createText(0, 0, "Hello World!", [1, 1, 1, 1], "Tavern");

sp.on("activate", () => {
  sp.setTextPos(textId, 200, 200);

  sp.setTextString(textId, "Hello World!");

  sp.setTextColor(textId, [1,1,1,1]);

  //new
  sp.setTextSize(textId, 3);

  sp.setTextRotation(textId, 45);

  sp.setTextFont(textId, "Tavern");

  sp.setTextDepth(textId, 1);

  sp.setTextEffect(textId, sp.SpriteEffects.FlipHorizontally);

  sp.setTextOrigin(textId, [0,0]);
});
```
