## Add function to disable Ctrl + PrtScn hotkey

This hotkey changes the game speed to be based on framerate rather than real time, which means the game runs normal speed at 30 FPS. Higher than that, the game runs too fast (2x speed at 60 FPS, 4x at 120, etc.) and lower than that, it runs slower. Since not everyone can maintain a constant 60 FPS (framedrops in Riften are common), this hotkey is not allowed.

[Source](https://www.thegamer.com/skyrim-tricks-work-banned/)

Now SP has a method to disable this hotkey:
```ts
import * as sp from "skyrimPlatform";

sp.disableCtrlPrtScnHotkey();
```
