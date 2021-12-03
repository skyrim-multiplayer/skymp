# Hooks (events)

Hooks allow you to intercept the start and end of some functions of the game engine.

Currently supported hooks: `sendAnimationEvent`, `sendPapyrusEvent`

## sendPapyrusEvent

Use this hook for catching standard [Papyrus events][PapyrusEvents].

```typescript
import { hooks, printConsole } from "skyrimPlatform";

// Subscribe to ALL Papyrus events
hooks.sendPapyrusEvent.add({
  enter(ctx) {
    printConsole(ctx.papyrusEventName);
  }
});

// Subscribe to OnSleepStart on player
hooks.sendPapyrusEvent.add({
  enter(ctx) {
    printConsole("Player started sleeping");
  },
  0x14, 0x14,
  "OnSleepStart"
});
```

- `enter` is called before starting the function. `ctx` contains the arguments passed to the function.
- `ctx.papyrusEventName`. Name of the Papyrus event catched by the hook.

### Caveats

At the moment of writing this, it's not possible to get arguments from Papyrus events. It's a [known issue][NoPapyrusEventArgs].\
Try to use [new events][NewEvents] whenever possible for the time being.

## sendAnimationEvent

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

Hooking is expensive: we need to enter the JavaScript context every time we enter/leave hooked function. SkyrimPlatform side can filter out some events before they will reach JS code. It allows doing less expensive JS calls. So the more we can check before triggering JS callback, the better.

In other words: specify `minSelfId`, `maxSelfId`, and `eventPattern` when possible to filter out unneeded events. It makes your code faster.

  ```typescript
  import { hooks, printConsole } from "skyrimPlatform";

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
## Removing a Hook
If you want to remove a hook, you must first save its ID when the hook is added, like so:

```typescript
const id = hooks.sendAnimationEvent.add({...});
//later...
hooks.sendAnimationEvent.remove(id);
```
This makes it possible to add and remove hooks dynamically based on [new events](https://github.com/skyrim-multiplayer/skymp/blob/main/docs/skyrim_platform/new_events.md).

For example, you could hook player animations under a spell:

```typescript
var id;

export let main = () => {
  on('effectStart', () => {
    id = hooks.sendAnimationEvent.add({...});
  });
  
  on('effectFinish', () => {
    if (id) hooks.sendAnimationEvent.remove(id);
  });
};

```

Note that nested hooks are **not** allowed.


## Contributing New Hooks

You might want to extend the default hooking functionality of SP by adding custom hooks in C++.

1) Hook a function ([click][FridaHooks]). Good example is `HOOK_SEND_ANIMATION_EVENT`. That's enough to hook something. The next step is to add the hook to our TypeScript API.
2) Create Enter/Leave methods in EventsApi like this ([click][EventsApi])
3) Add your hook to the list of hooks ([click][HooksList])
4) Add your hook to TypeScript definitions so it would be able to appear in skyrimPlatform.ts ([click][HookTs])
5) Pull request (see [CONTRIBUTING.md][])

[CONTRIBUTING.md]: https://github.com/skyrim-multiplayer/skymp/blob/main/CONTRIBUTING.md
[EventsApi]: https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/EventsApi.cpp#L354
[FridaHooks]: https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/FridaHooks.cpp
[HooksList]: https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/skyrim_platform/EventsApi.cpp#L411
[HookTs]: https://github.com/skyrim-multiplayer/skymp/blob/bf88abcc1922bbbfc12e177e522453f95eb60113/skyrim-platform/src/platform_se/codegen/convert-files/Definitions.txt#L538
[NewEvents]: new_events.md
[NoPapyrusEventArgs]: https://github.com/skyrim-multiplayer/skymp/issues/405
[PapyrusEvents]: https://www.creationkit.com/index.php?title=Category:Events
