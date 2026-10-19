# Events System

The events system lets your gamemode react to client-side situations by:

1. Defining an event source with `mp.makeEventSource(...)`.
2. Handling that event on the server with `mp._eventName = (...) => { ... }`.

## Event Naming

Custom event names must start with an underscore, for example `_onSomeEvent`.

## Basic Flow

```typescript
// 1) Register a client-side event source
mp.makeEventSource(
  '_onSomeEvent',
  `
  ctx.sp.once("update", () => {
    ctx.sendEvent({ foo: "bar" });
  });
`,
);

// 2) Register a server-side handler
mp._onSomeEvent = (pcFormId, payload) => {
  console.log('handled for', pcFormId, payload);
};
```

## Event Source Context Notes

In event-source code (`makeEventSource` body):

- `ctx.sendEvent(...)` is available and sends custom payload to server.
- `ctx.get(...)` is not available.
- `ctx.respawn()` is not available.

## Practical Example (Death Pulse)

```typescript
mp.makeEventSource(
  '_onLocalDeath',
  `
  ctx.sp.on("update", () => {
    const pl = ctx.sp.Game.getPlayer();
    const isDead = pl.getActorValuePercentage("health") === 0;

    if (ctx.state.wasDead !== isDead) {
      if (isDead) {
        ctx.sendEvent({ reason: "health_zero" });
      }
      ctx.state.wasDead = isDead;
    }
  });
`,
);

mp._onLocalDeath = (pcFormId, payload) => {
  console.log('downed', pcFormId, payload);
};
```

See also:

- [Serverside Scripting Reference](docs_serverside_scripting_reference.md)
- [Clientside Scripting Reference](docs_clientside_scripting_reference.md)
- Built-in pickup hook `mp.onItemPickupAttempt(...)` (see Serverside Scripting Reference)
