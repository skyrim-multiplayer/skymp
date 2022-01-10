# API for rendering text in skyrim

Skyrim now supports rendering text and has many methods for editing it.

## Example
```typescript
skyrimPlatform.createText(600, 600, "Hello", [1,1,0,1])  //Russian language is not yet supported
```
The amount of generated text is limited(5000 texts).
## API methods
1)```skyrimPlatform.createText(xpos, ypos, "string", ["array of RGBA colors"])``` - create text.
Returns id.

2)```skyrimPlatform.destroyText(textId)``` - delete text by id.
Return nothing.

3)```skyrimPlatform.setTextPos(textId, xpos, ypos)``` - set the position of the text by id.
Return nothing.

4)```skyrimPlatform.setTextString(textId, "string")``` - set text by id.
Return nothing.

5)```skyrimPlatform.setTextColor(textId, ["array of RBGA colors"])``` - set text color.
Return nothing.

6)```skyrimPlatform.destroyAllTexts()``` - delete all text.
Return nothing.

7)```skyrimPlatform.getTextPos(textId)``` - returns the coordinates(position) of the next as an array.

8)```skyrimPlatform.getTextString(textId)``` - returns a string.

9)```skyrimPlatform.getTextColor(textId)``` - returns an array of colors in RGBA.

10)```skyrimPlatform.getTextCount()``` - returns the number of created texts as a number.
