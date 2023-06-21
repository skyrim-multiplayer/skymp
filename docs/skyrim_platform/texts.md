# Texts API

Skyrim Platform now supports rendering texts and has methods for manipulating them.

`Data/Platform/Fonts/Tavern.spritefont` is used as a font.
`*.spritefont` is a custom format that DirectXTK library uses.
You can convert a `*.ttf` to this format using MakeSpriteFont utility that is
provided by DirectXTK. See [Compiling Font](#compiling-font) for details.

## Example

```typescript
skyrimPlatform.createText(0, 0, "Hello World!", [1, 1, 1, 1], "Tavern"),; // 0,0 is top left. Non-ASCII character are not yet supported.
skyrimPlatform.browser.setVisible(true); // Texts API takes visibility flag from the browser
```

## Complex

This simple API can be used to create dynamic texts attached to 3D points, objects, or even actor bones.
For example, SkyMP uses this API to draw nicknames  at `NPC Head [Head]` node
([#783](https://github.com/skyrim-multiplayer/skymp/pull/783/files)).

## Colors

Colors are represented as RGBA arrays (from 0 to 1).

```typescript
const white = [1,1,1,1];
```

## API methods

- ```skyrimPlatform.createText(xpos, ypos, "string", ["array of RGBA colors"], fontName: string)``` - create a text.
Returns id.

- ```skyrimPlatform.destroyText(textId)``` - delete text by id.
- ```skyrimPlatform.destroyAllTexts()``` - delete all texts.

### Setters

- ```skyrimPlatform.setTextPos(textId, xpos: float, ypos: float)``` - set the position of the text by id.

- ```skyrimPlatform.setTextString(textId, text: string)``` - set text by id.

- ```skyrimPlatform.setTextColor(textId, ["array of RBGA colors 0-1"])``` - set text color.

- ```skyrimPlatform.setTextSize(textId: number, size: float)``` - set text (not font) size.

- ```skyrimPlatform.setTextRotation(textId, rotation: float)``` - set text rotation.

- ```skyrimPlatform.setTextFont(textId, name: string)``` - set text font from relative path "Data/Platform/Fonts/", by font name — "Tavern".

- ```skyrimPlatform.setTextDepth(textId, depth: int)``` - set text z-index of the text.

- ```skyrimPlatform.setTextEffect(textId, effect: skyrimPlatform.SpriteEffects)``` - set sprite effect [None = 0, FlipHorizontally = 1, FlipVertically = 2].

- ```skyrimPlatform.setTextOrigin(textId, origin [x,y])``` - set text pivot (center point).

### Getters

- ```skyrimPlatform.getTextPos(textId)``` - returns the coordinates(position) of the next as an array.

- ```skyrimPlatform.getTextString(textId)``` - returns a string.

- ```skyrimPlatform.getTextColor(textId)``` - returns an array of colors in RGBA.

- ```skyrimPlatform.getTextSize(textId)``` - returns the size of the text

- ```skyrimPlatform.getTextRotation(textId)``` - returns the rotation of the text

- ```skyrimPlatform.getTextFont(textId)``` - returns the path to the font

- ```skyrimPlatform.getTextDepth(textId)``` - returns z-index of the text

- ```skyrimPlatform.getTextEffect(textId)``` - returns effect enum.

- ```skyrimPlatform.getTextOrigin(textId)``` - returns pivot (center point) of the text

- ```skyrimPlatform.getNumCreatedTexts()``` - returns the number of created texts.

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
