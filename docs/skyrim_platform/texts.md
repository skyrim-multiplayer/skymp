# Texts API

Skyrim Platform now supports rendering texts and has methods for manipulating them.

## Example
```typescript
skyrimPlatform.createText(600, 600, "Hello", [1,1,0,1])  // Non-ASCII character are not yet supported
```

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
