# SP 2.9.0 Release Notes

Thank you for using Skyrim Platform!

This document includes changes made since SP 2.8.0

WARNING: New game required. Update on current save at your own risk. Please backup your savegames.

This document will tell you more about project status: https://github.com/skyrim-multiplayer/skymp/blob/main/skyrim-platform/README.md
## Other changes

- After a series of individual fixes, all remaining unsafe event handlers have been resolved. **Crashes when loading a save should no longer occur.** ❤️‍🩹


- Added support for Skyrim 1.6.1170.0 ❤️‍🩹


- Added `TESModPlatform.CreateReferenceAtLocation` native


- Fixed crash when loading a save with at least one active `effectStart`/`effectFinish` listener.


- Slightly optimized Texts API.


- Fixed `playerBowShot` event lacking `isSunGazing` value.


- Taught `loadGame` function to work with non vanilla headparts.


- `blockPapyrusEvents` function now will not block events in `defaultDisableHavokOnLoad` script.


- Significantly optimized settings API.


- Added `TESModPlatform.CloseMenu` native


- `setInventory` now doesn't deny unequipping equipped items by default for the player (still denies for other actors).


- Fixed crash during hot-reload after an attempt to register a non-existent event listener.


- Added `blockPapyrusEvents` method as an alias for `TESModPlatform.blockPapyrusEvents` which is a Papyrus function and requires `on('update')` context to run.


- [Embedded console](https://github.com/skyrim-multiplayer/skymp/blob/592c6527ed91e6c97a38d143f4ae1cdab9c3268e/docs/release/sp-2.8.md?plain=1#L68) now mirrors unhandled exceptions from the game console properly.


- Added `time` and `loadOrder` parameters to `loadGame` function.


- Fixed runtime errors when bundlers treat const enums in skyrimPlatform.ts as non-const.


- Added initial support for calling script functions in tick context: `Game.getModCount`, `Game.getModName`.


- Fixed potential crash when loading a save with at least one active `magicEffectApply` listener.


- Added experimental `TESModPlatform.EvaluateLeveledNpc` native. It is unstable and shouldn't be used in user plugins. This native is required for SkyMP.


- Fixed crash in `CameraStateChanged` event when the player opens world map.


- HTTP Client API now supports callbacks, not only promises. This is useful since in SkyrimPlatform promises aren't resolving in the main menu.
