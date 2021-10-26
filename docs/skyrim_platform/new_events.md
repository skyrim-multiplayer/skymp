# Events

Aside from being able to subscribe to Papyrus events, Skyrim Platform also introduces events on its own.

Many of these events are are analogous to events already available in Skyrim, but these are easier to use in Typescript due to all extended and typed info they get.


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
The variable `event` always contains variables related to the event to which you are subscribed.

# List of new events
- At the moment, Skyrim Platform has the ability to subscribe to your own events: `update` and` tick`.

- `update` is an event that is called once for every frame in the game (60 times per second at 60 FPS) after you've loaded a save or started a new game.

  ```typescript
  import { on } from "skyrimPlatform";
  on("update", () => {
    // At this stage, the methods of all imported
    // types are already available.
  });
  ```

- `tick` is an event that is called once for every frame in the game immediately after the game starts.
  ```typescript
  import { on } from "skyrimPlatform";
  on("tick", () => {
    // No access to game methods here.
  });
  ```
- And also for game events such as `effectStart`, `effectFinish`, `magicEffectApply`, `equip`, `unequip`, `hit`, `containerChanged`, `deathStart`, `deathEnd`, `loadGame`,  `combatState`, `reset`, `scriptInit`, `trackedStats`, `uniqueIdChange`, `switchRaceComplete`, `cellFullyLoaded`, `grabRelease`, `lockChanged`, `moveAttachDetach`, `objectLoaded`, `waitStop`, `activate` ...
