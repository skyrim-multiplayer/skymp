# Texts API

Skyrim Platform now supports rendering texts and has methods for manipulating them.

`Data/Platform/Fonts/font.spritefont` is used as a font.
See [MakeSpriteFont](https://github.com/microsoft/DirectXTK/wiki/MakeSpriteFont) if you want to compile your font into spritefont format.

## Example
```typescript
// 0,0 is top left. Non-ASCII character are not yet supported.
skyrimPlatform.createText(0, 0, "Hello", [1,1,0,1]);
```

## Complex

This simple API can be used to create dynamic texts attached to 3D points, objects, or even actor bones.
For example, SkyMP uses this API to draw nicknames  at `NPC Head [Head]` node.
([#783](https://github.com/skyrim-multiplayer/skymp/pull/783/files))

## Colors

Colors are represented as RGBA arrays (from 0 to 1).
```typescript
const white = [1,1,1,1];
```

## API methods

1) ```skyrimPlatform.createText(xpos, ypos, "string", ["array of RGBA colors"])``` - create a text.
Returns id.

2) ```skyrimPlatform.destroyText(textId)``` - delete text by id.

3) ```skyrimPlatform.setTextPos(textId, xpos, ypos)``` - set the position of the text by id.

4) ```skyrimPlatform.setTextString(textId, "string")``` - set text by id.

5) ```skyrimPlatform.setTextColor(textId, ["array of RBGA colors"])``` - set text color.

6) ```skyrimPlatform.destroyAllTexts()``` - delete all texts.

7) ```skyrimPlatform.getTextPos(textId)``` - returns the coordinates(position) of the next as an array.

8) ```skyrimPlatform.getTextString(textId)``` - returns a string.

9) ```skyrimPlatform.getTextColor(textId)``` - returns an array of colors in RGBA.

10) ```skyrimPlatform.getNumCreatedTexts()``` - returns the number of created texts.
