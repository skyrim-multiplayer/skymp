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
