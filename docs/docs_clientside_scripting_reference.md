# Clientside Scripting Reference

In Skyrim Multiplayer there are no dedicated clientside scripts. Code snippets that runs client-side are passed as strings to `makeProperty`, `updateOwner` and `updateNeighbor` which are described in [Serverside Scripting Reference](docs_serverside_scripting_reference.md).

## ctx.sp

Refers to Skyrim Platform API. See [Skyrim Platform](docs_skyrim_platform.md) page.

```typescript
// Print to console
ctx.sp.printConsole("Hello Skyrim Platform!");
// Kill player character (locally)
ctx.sp.Game.getPlayer().kill();
```

## ctx.refr

In `makeProperty` is always `undefined`.

In `updateOwner` is similar to `ctx.sp.Game.getPlayer()`.

In `updateNeighbor` refers to neighbor synchronized `ObjectReference` or `Actor`.

```typescript
const pos = [
  ctx.refr.getPositionX(),
  ctx.refr.getPositionY(),
  ctx.refr.getPositionZ()
];
```

## ctx.value

In `makeProperty` is always `undefined`.

In `updateOwner` / `updateNeighbor` is equal to the value of a property that is processed currently or `undefined` if there is no value or it's not visible due to flags.

```typescript
ctx.sp.Game.setPlayerLevel(ctx.value || 1);
```

## ctx.state

A writable object that is used to store data between `updateOwner`/`updateNeighbor` calls or `makeProperty` initializations.

`state` is currently shared between properties.

```typescript
ctx.state.x = "y";
```

## ctx.get()

Get the value of the specified property. Built-in properties are not supported properly, so attempts getting them are leading to the undefined behavior.

```typescript
const v = ctx.get("myAwesomeProperty");
```

## ctx.getFormIdInServerFormat()

Gets serverside formId by clientside formId or `0` if not found.

```typescript
const serversideFormId = ctx.getFormIdInServerFormat(0xff00016a);
```

## ctx.getFormIdInClientFormat()

Opposite to `getFormIdInServerFormat`. Gets clientside formId by serverside formId or 0 if not found.

```typescript
const clientsideFormId = ctx.getFormIdInClientFormat(0xff000000);
```

## ctx.respawn()

Respawns `ctx.refr` immediately.

```typescript
ctx.respawn();
```

## ctx.sendEvent()

Available only in `makeProperty` context. Sends an event to the server.

```typescript
ctx.sendEvent({ foo: 'bar' });
```
