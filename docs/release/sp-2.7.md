# SP 2.7.0 Release Notes


This document includes changes made since SP 2.6.0


SP updates regularly. This update probably doesn't include ALL patches that have to be made.

There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

SP supports both Steam version of Skyrim SE (1.6+) and legacy 1.5.97.

## Handle attempts to call non-native Papyrus functions

Currently, Skyrim Platform supports only calls to non-native functions. The only change is that we now throw a JavaScript exception instead of crashing.

```js
// JS example, in TS we already couldn't call non-natives :p
sp = skyrimPlatform;
sp.on("update", () => {
  sp.printConsole(sp.Game.getplayer().getActorValuePercentage("health")); 
  // ^ "1"

  sp.printConsole(sp.Game.getplayer().getAVPercentage("health")); 
  // ^ "[Exception] Function is not native 'Actor.getAVPercentage'"
  // No crash in the new version
});
```


## Other changes

- Fixed unexpectedly created folders in "overwrite" under MO2.


- Added Skyrim 1.6.640 support.


- Improved the detection of plugins, avoiding the interpretation of files with double extensions or with the extension ".DS_Store" as JavaScript code.


- Updated Chromium Embedded Framework to v105.


- Fixed double cursor when the browser is focused and visible.


- Added `InstallApplyMasksToRenderTargetsHook` back after removing by mistake.


- Added `setInventory` method to avoid CTDs on mass inventory operations when using `TESModPlatform.AddItemEx`.


- Fixed frequent freezes related to scripting functions `removeAllItems`, `equipSpell`, and other.


- Fixed CI script that added excess files to SkyrimPlatform Anniversary Edition archive.


- Added back the "a**g**ressor" field of hit event. It should be still there for backward compatibility of compiled plugins. Only the 2.6 version was affected by the bug. This typo will be completely removed from the project in 3.0.0.
