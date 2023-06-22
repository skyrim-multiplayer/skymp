# SP 2.3 Release Notes

This document includes changes made since SP 2.2.

SP updates regularly. This update probably doesn't include ALL patches that have to be made.
There are still many things to be implemented or fixed. See [issues](https://github.com/skyrim-multiplayer/skymp/issues?q=is%3Aopen+is%3Aissue+label%3Aarea%3Askyrim-platform).

Please note that the current SP version only works for the old SE build (before the 11.11.21 update).
To downgrade your Skyrim SE installation use [this patch](https://www.nexusmods.com/skyrimspecialedition/mods/57618).

## Add `hook.remove` method

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

## Fix vulnerability in `writeLogs` method

Without the fix, we were able to write text files wherever we wanted in the host system:

```typescript
writeLogs("../foo", "111");
// or even:
writeLogs("../../foo", "111");
```
Currently, SP doesn't allow writing/reading file system by plugins except for the Data folder. It's by design. However, PapyrusUtil/JContainers/other plugins can expose everything and that's ok.

## Other changes

- `writeLogs` API now writes to `Data/Platform/Logs` instead of `Data/Platform/Plugins`
- Added ability to load plugins from additional folder: `Platform/PluginsDev`. Supports hot reloading for MO2 users with downloaded SP plugins.
