# Events

- At the moment, SkyrimPlatform has the ability to subscribe to your own events: `update` and` tick`.

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

## Subscribing to events

- With `on`, you can subscribe to the event forever.
  ```typescript
  import { on } from "skyrimPlatform";
  on("equip", (event) => {
    printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
    printConsole(`object: ${event.baseObj.getName()}`);
  });
  ```
- Using `once`, you can add a handler that will be called once the next time the event is fired.
  ```typescript
  import { once } from "skyrimPlatform";
  once("equip", (event) => {
    printConsole(`actor: ${event.actor.getBaseObject().getName()}`);
    printConsole(`object: ${event.baseObj.getName()}`);
  });
  ```
- The variable `even` always contains variables related to the event to which you are subscribed.

# Hooks

- Hooks allow you to intercept the start and end of some functions of the game engine.
- Currently supported hooks: `sendAnimationEvent`, `sendPapyrusEvent`
  ```typescript
  import { hooks, printConsole } from "skyrimPlatform";
  hooks.sendAnimationEvent.add({
    enter(ctx) {
      printConsole(ctx.animEventName);
    },
    leave(ctx) {
      if (ctx.animationSucceeded) printConsole(ctx.selfId);
    }
  });
  ```
- `enter` is called before starting the function. `ctx` contains the arguments passed to the function and also` storage` (see below).
- `leave` is called before the function ends. `ctx` contains the return value of the function, in addition to what was after the completion of` enter`.
- `ctx` is the same object for calls to` enter` and `leave`.
- `ctx.storage` is used to store data between calls to` enter` and `leave`.
- Script functions are not available inside the `enter` and` leave` handlers.

## Advanced Hooking

- Hooking is expensive: we need to enter the JavaScript context every time we enter/leave hooked function. SkyrimPlatform side can filter out some events before they will reach JS code. It allows doing less expensive JS calls. So the more we can check before triggering JS callback, the better.
- In other words: specify `minSelfId`, `maxSelfId`, and `eventPattern` when possible to filter out unneeded events. It makes your code faster.
  ```typescript
  import { hooks, printConsole } from "skyrimPlatform"

  // Bad. Catches all animation events in the world when we only want to catch events from the player character.
  hooks.sendAnimationEvent.add({
    enter(ctx) {
      if (ctx.selfId !== 0x14) return;
      printConsole("Player's anim:", ctx.animEventName);
    },
    leave(ctx) {}
  });

  // Good. No JS is triggered until selfId in range `[minSelfId..maxSelfId]` found and event name matches `"*"` wildcard.
  // A maximum of one wildcard can be used: for example, the *Sleep* pattern won't work.
  // Two or more wildcards are not supported and will result in an exception.
  hooks.sendAnimationEvent.add({
    enter(ctx) {
      printConsole("Player's anim:", ctx.animEventName);
    },
    leave(ctx) {}
  }, /* minSelfId = */ 0x14, /* maxSelfId = */ 0x14, /*eventPattern = */ "*");
  ```

## Contributing New Hooks

You might want to extend the default hooking functionality of SP by adding custom hooks in C++.

1) Hook a function ([click](https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/FridaHooks.cpp)). Good example is `HOOK_SEND_ANIMATION_EVENT`. That's enough to hook something. The next step is to add the hook to our TypeScript API.
2) Create Enter/Leave methods in EventsApi like this ([click](https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/EventsApi.cpp#L354))
3) Add your hook to the list of hooks ([click](https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/EventsApi.cpp#L411))
4) Add your hook to TypeScript definitions so it would be able to appear in skyrimPlatform.ts ([click](https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/codegen/convert-files/Definitions.txt#L538))
5) Pull request (see [CONTRIBUTING.md](https://github.com/skyrim-multiplayer/skymp/blob/main/CONTRIBUTING.md))
