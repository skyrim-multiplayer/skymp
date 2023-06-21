# Texts API

Skyrim Platform now supports rendering texts and has methods for manipulating them.

`Data/Platform/Fonts/Tavern.spritefont` is used as a font.
`*.spritefont` is a custom format that DirectXTK library uses.
You can convert a `*.ttf` to this format using MakeSpriteFont utility that is
provided by DirectXTK. See [Compiling Font](#compiling-font) for details.

## Example

```typescript
skyrimPlatform.createText(0, 0, 'Hello', [1, 1, 0, 1]); // 0,0 is top left. Non-ASCII character are not yet supported.
skyrimPlatform.browser.setVisible(true); // Texts API takes visibility flag from the browser
```

## Complex

This simple API can be used to create dynamic texts attached to 3D points, objects, or even actor bones.
For example, SkyMP uses this API to draw nicknames at `NPC Head [Head]` node
([#783](https://github.com/skyrim-multiplayer/skymp/pull/783/files)).

## Colors

Colors are represented as RGBA arrays (from 0 to 1).

```typescript
const white = [1, 1, 1, 1];
```

## API methods

1. `skyrimPlatform.createText(xpos, ypos, "string", ["array of RGBA colors"])` - create a text.
   Returns id.

2. `skyrimPlatform.destroyText(textId)` - delete text by id.

3. `skyrimPlatform.setTextPos(textId, xpos, ypos)` - set the position of the text by id.

4. `skyrimPlatform.setTextString(textId, "string")` - set text by id.

5. `skyrimPlatform.setTextColor(textId, ["array of RBGA colors"])` - set text color.

6. `skyrimPlatform.destroyAllTexts()` - delete all texts.

7. `skyrimPlatform.getTextPos(textId)` - returns the coordinates(position) of the next as an array.

8. `skyrimPlatform.getTextString(textId)` - returns a string.

9. `skyrimPlatform.getTextColor(textId)` - returns an array of colors in RGBA.

10. `skyrimPlatform.getNumCreatedTexts()` - returns the number of created texts.

## Compiling Font

To compile a font, you need `MakeSpriteFont.exe` utility from DirectXTK releases
and to have a `*.ttf` installed in your system.

```
./MakeSpriteFont.exe "Typey McTypeface" Tavern.spritefont /FontSize:20 /CharacterRegion:32-126 /CharacterRegion:1040-1103 /CharacterRegion:1025 /CharacterRegion:1105 /DefaultCharacter:63
```

- `"Typey McTypeface"` is the internal name of `Tavern.ttf` (input font)
- `Tavern.spritefont` is filename for output font
- `/CharacterRegion:32-126 /DefaultCharacter:63` - default ASCII characters region with 63 (`?`) as a fallback character
- `/CharacterRegion:1040-1103 /CharacterRegion:1025 /CharacterRegion:1105` - Russian alphabet

You can use Python (or whatever tool you like, including
[web-services](https://onlineutf8tools.com/convert-utf8-to-code-points))
to find out characters mapping:

```
$ python3
>>> ord('Ё')
1025
>>> chr(1105)
'ё'
```

More info and downloads:

- [DirectXTK wiki](https://github.com/microsoft/DirectXTK/wiki/MakeSpriteFont)
- [Download `MakeSpriteFont.exe` from DirectXTK releases page](https://github.com/microsoft/DirectXTK/releases)
