# Events

Aside from being able to subscribe to Papyrus events, Skyrim Platform also introduces events on its own.

Many of these events are analogous to events already available in Skyrim, but these are easier to use in Typescript due to all extended and typed info they get.

Skyrim Platform does not attach scripts to objects, so these events are available without the need to do so.

## Subscribing to events

With `on`, you can subscribe to an event forever.

```typescript
import { on } from "skyrimPlatform";

on("equip", (event) => {
  printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
  printConsole(`object: ${event.baseObj.getName()}`);
});
```

Using `once`, you can add a handler that will be called once the next time the event is fired.

```typescript
import { once } from "skyrimPlatform";

once("equip", (event) => {
  printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
  printConsole(`object: ${event.baseObj.getName()}`);
});
```

Both `on` and `once` return an `EventHandle` object, which contains unique subscription id and event name: `{ uid, eventName }`.
Using `EventHandle` is optional, but it is required if you want to unsubscribe from the event later in your code.

```typescript
import { on, unsubscribe } from "skyrimPlatform";

const handle = on("equip", (event) => {
  printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
  printConsole(`object: ${event.baseObj.getName()}`);
});

unsubscribe(handle);
```

The variable `event` contains variables related to the event (*if they exist*) to which you are subscribed.

# List of new events

  - [update](#update)
  - [tick](#tick)
  - [equip](#equip)
  - [unequip](#unequip)
  - [effectStart](#effectstart)
  - [effectFinish](#effectfinish)
  - [cellFullyLoaded](#cellFullyLoaded)
  - [consoleMessage](#consoleMessage)
  - [loadGame](#loadgame)
  - [Other events](#other-events)

 ## update

 Called once for every frame in the game (60 times per second at 60 FPS) after you've loaded a save or started a new game.

```typescript
import { on } from "skyrimPlatform";

on("update", () => {
  // At this stage, the methods of all imported
  // types are already available.
});
```

***NOTE***: Executing code each single frame may be expensive depending on what you are doing.

For `update` and `tick` try to use `once` instead of `on`, if possible.

It's also a good idea to try to use any of the events below or even [Papyrus Hooks][Events] before considering executing code each frame.

## tick

Called once for every frame in the game immediately after the game starts.

***WARNING***: This event has no access to [game methods and objects][Papyrus].

```typescript
import { on } from "skyrimPlatform";

on("tick", () => {
  // No access to game methods here.
});
```

## equip

Called each time any `Actor` equips an item.\
Analogous to [OnObjectEquipped][OnObjectEquipped].

```typescript
import { on } from "skyrimPlatform";

on("equip", (event) => {
  const b = event.actor.getBaseObject();
  const armor = Armor.from(event.baseObj);

  if (!armor || armor.getSlotMask() !== 0x4 || !b) return;

  printConsole(`ActorBase ${b.getName()} equipped this cuirass: ${armor.getName()}`);
})
```

## unequip

Called each time any `Actor` unequips an item.\
Analogous to [OnObjectUnequipped][OnObjectUnequipped].

```typescript
import { on } from "skyrimPlatform";

on("unequip", (event) => {
  if (event.actor.getFormID() !== Game.getPlayer()?.getFormID()) return;

  const w = Weapon.from(event.baseObj);

  if (w) {
    printConsole("Player unequipped a weapon");
  }
})
```

## effectStart

Called when any Magic Effect starts.\
Analogous to [OnEffectStart][OnEffectStart].

***HINT***: If you are using cloaks to apply Magic Effects in your mod, you can drastically increase performance by applying a blank Magic Effect with [SPID][SPID] instead of cloaks and letting Skyrim Platform deal with the real code:

```typescript
import { on } from "skyrimPlatform";

on("effectStart", (event) => {
  const fx = Game.getFormFromFile(0x800, "my-mod.esp");
  if (fx?.getFormID() !== event.effect.getFormID()) return;

  DoSomething(event.target);
})
```

See the Cook Book [entry on this][Cloaks] for more details.

## effectFinish

Called when any Magic Effect ends.\
Analogous to [OnEffectFinish][OnEffectFinish].

```typescript
import { on } from "skyrimPlatform";

on("effectFinish", (event) => {
  printConsole(`${event.effect.getName()} finished on ${event.target.getName()}`);
})
```

## cellFullyLoaded

Called each time a game cell has finished loading.

```typescript
import { on } from "skyrimPlatform";

on("cellFullyLoaded", (event) => {
  printConsole(`Cell "${event.cell.getName()}" was fully loaded`);
})
```

Notice that due to the way Skyrim works this event may fire multiple times when roaming the wilderness, since many cells are usually loaded at the same time in this situation.

This event will be called once for each of those cells that just had been loaded.

## consoleMessage

Called each time the game prints something to the console, including calls to `printConsole`.

Note: The message text can contain any characters, including `'` `"` `\`.
Before sending the text to the browser using "browser.executeJavaScript", it should be escaped.

```typescript
import { on, browser } from "skyrimPlatform";

const htmlEscapes: Record<string, string> = {
  '"': '\\"',
  "'": "\\'",
  '\\': '\\\\',
  '<': '\\<',
  '>': '\\>'
};

const htmlEscaper = /[&<>"'\\\/]/g;

// On every print to the game console, console.log it to the browser
on('consoleMessage', (e) => {
  const msg = e.message.replace(htmlEscaper, (match) => htmlEscapes[match]);
  browser.executeJavaScript('console.log("' + msg + '")');
});
```

## loadGame

Called when a saved game is loaded.

***WARNING***: This event won't fire when creating a new game. Use `newGame` for that.\
Or [read this recipe][PluginInit] to get some ideas on how you can deal with plugin initialization.

## Other events

More info and samples for these will be added later:

- action event group
  - `actionBeginDraw`
  - `actionBeginSheathe`
  - `actionBowDraw`
  - `actionBowRelease`
  - `actionEndDraw`
  - `actionEndSheathe`
  - `actionSpellCast`
  - `actionSpellFire`
  - `actionVoiceCast`
  - `actionVoiceFire`
  - `actionWeaponSwing`
- `activate`
- `actorKill`
- `bookRead`
- `browserMessage`
- input event group
  - `buttonEvent`
  - `mouseMove`
  - `thumbstickEvent`
  - `kinectEvent`
  - `deviceConnect`
- `cameraStateChanged`
- cell event group
  - `cellAttach`
  - `cellDetach`
  - `cellFullyLoaded`
- `combatState`
- `containerChanged`
- `criticalHit`
- `crosshairRefChanged`
- death event group
  - `deathStart`
  - `deathEnd`
- `destructionStageChanged`
- `disarmedEvent`
- `dragonSoulsGained`
- `enterBleedout`
- `fastTravelEnd`
- `footstep`
- furniture event group
  - `furnitureEnter`
  - `furnitureExit`
- `grabRelease`
- `hit`
- `ipcMessage`
- `itemHarvested`
- `levelIncrease`
- location event group
  - `locationChanged`
  - `locationDiscovery`
- `lockChanged`
- `magicEffectApply`
- menu event group
  - `menuOpen`
  - `menuClose`
- SKSE
  - `modEvent`
  - `niNodeUpdate`
  - `skyrimLoaded`
- `moveAttachDetach`
- `objectLoaded`
- open\close event group
  - `open`
  - `close`
- package event group
  - `packageStart`
  - `packageEnd`
  - `packageChange`
- `perkEntryRun`
- `playerBowShot`
- `positionPlayer`
- quest event group
  - `questInit`
  - `questStart`
  - `questStop`
  - `questStage`
- `reset`
- save\load event group
  - `saveGame`
  - `loadGame`
  - `postLoadGame`
  - `preLoadGame`
  - `deleteGame`
  - `newGame`
- `sceneAction`
- `scriptInit`
- `sell`
- `shoutAttack`
- `skillIncrease`
- `sleepStart`
- `sleepStop`
- `soulsTrapped`
- `spellCast`
- `spellsLearned`
- `switchRaceComplete`
- `trackedStats`
- translation event group
  - `translationAlmostCompleted`
  - `translationCompleted`
  - `translationFailed`
- trigger event group
  - `trigger`
  - `triggerEnter`
  - `triggerLeave`
- `uniqueIdChange`
- wait event group
  - `waitStart`
  - `waitStop`
- `wardHit`

[Cloaks]: cookbook.md#getting-rid-of-cloaks
[Events]: events.md
[OnEffectFinish]: https://www.creationkit.com/index.php?title=OnEffectFinish_-_ActiveMagicEffect
[OnEffectStart]: https://www.creationkit.com/index.php?title=OnEffectStart_-_ActiveMagicEffect
[OnObjectEquipped]: https://www.creationkit.com/index.php?title=OnObjectEquipped_-_Actor
[OnObjectUnequipped]: https://www.creationkit.com/index.php?title=OnObjectUnequipped_-_Actor
[Papyrus]: papyrus.md
[PluginInit]: cookbook.md#plugin-initialization
[SPID]: https://www.nexusmods.com/skyrimspecialedition/mods/36869
