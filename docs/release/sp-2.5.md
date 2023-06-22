# SP 2.5.0 Release Notes


This document includes changes made since SP 2.4.0


SP updates regularly. This update probably doesn't include ALL patches that have to be made.

There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

Please note that the current SP version only works for the old SE build (before the 11.11.21 update).

To downgrade your Skyrim SE installation use [this patch](https://www.nexusmods.com/skyrimspecialedition/mods/57618).

## Add `win32.exitProcess` method

This method allows you to close the game by calling `std::exit` under the hood:

```typescript
import { once, win32 } from "skyrimPlatform";

once("activate", () => {
  // Exit game on any activation
  win32.exitProcess();
});
```


## Add more events

Lots of new events were added to reflect vanilla Papyrus scripting abilities. For example:

```typescript
// OnPlayerBowShot TypeScript equivalent
on("playerBowShot", (event) => {
  printConsole(event.weapon.getFormID());
  printConsole(event.ammo.getFormID());
  printConsole(event.power);
  printConsole(event.isSunGazing);
});
```

Interfaces of added events can be easily explored via IDE IntelliSense, or directly in `skyrimPlatform.ts`.

Full list of added events:

- `open`
- `close`
- `questInit`
- `questStart`
- `questStop`
- `questStage`
- `trigger`
- `triggerEnter`
- `triggerLeave`
- `sleepStart`
- `sleepStop`
- `locationChanged`
- `bookRead`
- `sell`
- `furnitureEnter`
- `furnitureExit`
- `wardHit`
- `packageStart`
- `packageChange`
- `packageEnd`
- `enterBleedout`
- `destructionStageChanged`
- `sceneAction`
- `playerBowShot`
- `fastTravelEnd`
- `perkEntryRun`
- `translationFailed`
- `translationAlmostCompleted`
- `translationCompleted`
- `actionWeaponSwing`
- `actionBeginDraw`
- `actionEndDraw`
- `actionBowDraw`
- `actionBowRelease`
- `actionBeginSheathe`
- `actionEndSheathe`
- `actionSpellCast`
- `actionSpellFire`
- `actionVoiceCast`
- `actionVoiceFire`
- `cameraStateChanged`
- `crosshairRefChanged`
- `niNodeUpdate`
- `modEvent`
- `positionPlayer`
- `footstep`


## Add Texts API

Add API for creating texts via DirectX overlay.

```typescript
skyrimPlatform.createText(600, 600, "Hello Skyrim", [1,1,0,1]);
```

See [texts.md](https://github.com/skyrim-multiplayer/skymp/tree/main/docs/skyrim_platform/texts.md) for documentation.
