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
