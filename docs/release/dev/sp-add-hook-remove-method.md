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
