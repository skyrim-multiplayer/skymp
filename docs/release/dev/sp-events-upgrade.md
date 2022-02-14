## Improve event system

The event system has been redesigned. Now when you subscribe to an event, a handle object is returned.
Then you can unsubscribe from an event using the added `unsubscribe` method by passing a handle to it as an argument.

```typescript
import { on, unsubscribe } from "skyrimPlatform";

let handle = on('disarmedEvent', (event) => {
  printConsole(`disarmedEvent`);
  unsubscribe(handle);
})
```

The following events have also been added:
- `skyrimLoaded`
- `newGame`
- `preLoadGame`
- `postLoadGame`
- `saveGame`
- `deleteGame`
- `buttonEvent`
- `mouseMove`
- `thumbstickEvent`
- `kinectEvent`
- `deviceConnect`
- `actorKill`
- `criticalHit`
- `disarmedEvent`
- `dragonSoulsGained`
- `itemHarvested`
- `levelIncrease`
- `locationDiscovery`
- `shoutAttack`
- `skillIncrease`
- `soulsTrapped`
- `spellsLearned`