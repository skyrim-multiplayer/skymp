# Clientside Scripting Reference

In Skyrim Multiplayer there are no dedicated clientside scripts. Code snippets that runs client-side are passed as strings to `makeProperty`, `updateOwner` and `updateNeighbor` which are described in [Serverside Scripting Reference](docs_serverside_scripting_reference.md).

There is also event-source code provided via `mp.makeEventSource(...)`.

## Context Overview

The same `ctx` object shape is reused in different execution contexts, but not all members are available everywhere.

- `updateOwner`: code from `makeProperty(...).updateOwner`
- `updateNeighbor`: code from `makeProperty(...).updateNeighbor`
- `eventSource`: code from `makeEventSource(...)`

## ctx.sp

Refers to Skyrim Platform API. See [Skyrim Platform](docs_skyrim_platform.md) page.

```typescript
// Print to console
ctx.sp.printConsole('Hello Skyrim Platform!');
// Kill player character (locally)
ctx.sp.Game.getPlayer().kill();
```

## ctx.refr

In `makeProperty` is always `undefined`.

In `updateOwner` is similar to `ctx.sp.Game.getPlayer()`.

In `updateNeighbor` refers to neighbor synchronized `ObjectReference` or `Actor`.

In `eventSource` it is usually `undefined` unless your script assigns it manually in `ctx.state`.

```typescript
const pos = [
  ctx.refr.getPositionX(),
  ctx.refr.getPositionY(),
  ctx.refr.getPositionZ(),
];
```

## ctx.value

In `makeProperty` is always `undefined`.

In `updateOwner` / `updateNeighbor` is equal to the value of a property that is processed currently or `undefined` if there is no value or it's not visible due to flags.

In `eventSource` it is `undefined` by default.

```typescript
ctx.sp.Game.setPlayerLevel(ctx.value || 1);
```

## ctx.state

A writable object that is used to store data between `updateOwner`/`updateNeighbor` calls or `makeProperty` initializations.

`state` is currently shared between properties.

For event sources, each source has its own `ctx.state` object that persists between callback calls.

```typescript
ctx.state.x = 'y';
```

## ctx.get()

Get the value of the specified property. Built-in properties are not supported properly, so attempts getting them are leading to the undefined behavior.

`ctx.get(...)` is not available in `eventSource` context.

```typescript
const v = ctx.get('myAwesomeProperty');
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

In `updateNeighbor`, removes the current synchronized neighbor view entry.

`ctx.respawn()` is not available in `updateOwner` and `eventSource` contexts.

```typescript
ctx.respawn();
```

## ctx.sendEvent()

Available only in `eventSource` context. Sends an event to the server.

```typescript
ctx.sendEvent({ foo: 'bar' });
```

## Quick Availability Matrix

- `updateOwner`: `sp`, `refr`, `value`, `state`, `get`, form-id converters
- `updateNeighbor`: `sp`, `refr`, `value`, `state`, `get`, form-id converters, `respawn`
- `eventSource`: `sp`, `state`, form-id converters, `sendEvent`

See also [Events System](docs_events_system.md).
