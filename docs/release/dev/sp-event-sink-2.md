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
