# SkyMP Addon System Contract

This file captures the current addon contract while the design is still fresh.
It is intentionally small and practical. The goal is to make it clear what is
currently treated as public API, what is still transitional, and what a second
addon author would need to follow.

## Status

- Current API version: client `5`, server `3`
- Public API entry points live in:
  - `skymp5-addons-api/clientAddonHost.ts`
  - `skymp5-addons-api/serverAddonHost.ts`
- Current host/loader implementations live in:
  - `skymp5-client/src/services/services/clientAddonHostService.ts`
  - `skymp5-server/ts/addons/serverAddonLoader.ts`
Treat the `skymp5-addons-api` package as the stable surface first. Treat the
host/loader implementation details as internal unless they are promoted here.

## Client Addon Contract

Client addons currently self-register into a global host. An addon bundle calls
`ensureClientAddonHostGlobal().registerClientAddon(addonId, init)`, and the
host will either register immediately or queue the registration until the
runtime host is active.

### Registration rules

- `addonId` must be unique per client runtime.
- Empty addon IDs are rejected.
- Registration order is not guaranteed to be meaningful.

### Client API methods

Exposed by `ClientAddonApi` in `skymp5-addons-api/clientAddonHost.ts`:

- `auth.onAuthNeeded(handler)`
- `auth.onConnectionAccepted(handler)`
- `auth.onConnectionDenied(handler)`
- `auth.onConnectionDisconnect(handler)`
- `auth.onConnectionFailed(handler)`
- `auth.submitAuthAttempt(authGameData)`
- `capabilities`
- `browser.emitEvent(eventName, dataJson)`
- `browser.getBackendName()`
- `browser.isFocused()`
- `browser.isVisible()`
- `browser.loadUrl(url)`
- `browser.setFocused(focused)`
- `browser.setVisible(visible)`
- `getLocalProfileId()`
- `getSettingsScope(scope)`
- `log(...args)`
- `logError(...args)`
- `resolveScanCode(keyName)`
- `sendCustomPacket(type, payload, reliability?)`

### Client API subscriptions

- `onBrowserMessage(key, handler)`
- `onCustomPacket(type, handler)`
- `onInputState(handler)`
- `onLocalSpawn(handler)`
- `onTick(handler)`

All subscription methods return an unsubscribe function.

### Current client caveats

- Client addons are still normal JS bundles loaded by Skyrim Platform; there is
  not yet a generic client-side discovery/manifest system.
- The browser API is a stable logical surface, but the current host still relies
  on Skyrim Platform browser behavior and a DOM/custom-event fallback.

## Server Addon Contract

Server addons are config-driven modules loaded from
`settings.addonModules.server`. Each module exports:

- `createServerAddon(api, config)`
- optional `addonId`

If `addonId` is omitted, the loader derives one from the module filename.

### Server API methods

Exposed by `ServerAddonApi` in `skymp5-addons-api/serverAddonHost.ts`:

- `capabilities`
- `error(...args)`
- `getActorAngleZ(actorId)`
- `getActorCellOrWorld(actorId)`
- `getActorPos(actorId)`
- `getConfig<T>()`
- `getUserActor(userId)`
- `isConnected(userId)`
- `log(...args)`
- `onCustomPacket(type, handler)`
- `onSpawnAllowed(handler)`
- `sendCustomPacket(userId, type, payload)`
- `addonId`
- `version`

### Server addon hooks

Addons may implement:

- `init()` - may be async
- `connect(userId)` - synchronous
- `customPacket(userId, type, content)` - synchronous
- `disconnect(userId)` - synchronous
- `dispose()` - may be async
- `update()` - may be async
- `systemName`

The loader wraps these hooks into the normal server `System` lifecycle.
Configured addon load failures are fatal in production startup semantics: if a
module cannot be resolved, exports the wrong shape, duplicates an `addonId`, or
throws during `createServerAddon(api, config)`, the server does not start.
`onCustomPacket(type, handler)` is the preferred higher-level packet API for new
addons and is invoked synchronously, just like
`customPacket(userId, type, content)`. `sendCustomPacket(userId, type, payload)`
is the matching higher-level server-to-client path and serializes
`customPacketType` for the client host automatically.

### Server addon path resolution

Relative entries in `addonModules.server` resolve from the directory that
contains `server-settings.json`, not from `process.cwd()`.

## Config Entry Points

### Generic server config

Current generic server-side addon settings:

- `addonModules.server`
  - array of module paths to load
  - relative paths resolve from the `server-settings.json` directory
- `addons.<addonId>`
  - per-addon config object passed to `createServerAddon(api, config)`

### Client config

There is not yet a generic client addon config registry. The current
expectation is that each client addon owns its own top-level settings scope,
typically something like `sp.settings["skymp5-<addonId>"]`, and reads it
through `ClientAddonApi.getSettingsScope(scope)`.

If an addon needs migration compatibility with older settings keys, keep that
compatibility in an addon-local helper and document it in the addon's own
README. Do not treat migration shims as part of the generic core contract.

## Messaging And Namespacing Conventions

Addon authors must namespace any cross-boundary messages so different addons
do not collide.

### Custom packets

- Reserve `customPacketType` values per addon.
- Recommended pattern: `<addonId>:<messageName>`
- Keep packet parsing/creation in a shared addon-owned protocol module.
- Example pattern:
  - `inventory-sync:state`
  - `weather-tools:request`

### Browser event names

- Namespace browser event names per addon.
- Recommended pattern: `skymp5-<addonId>:<eventName>`
- Example pattern:
  - `skymp5-inventory-sync:ready`
  - `skymp5-weather-tools:refresh`

### DOM fallback attributes

- If an addon needs DOM attribute fallbacks, namespace them too.
- Recommended pattern: `data-skymp-<addonId>-...`
- Example pattern:
  - `data-skymp-inventory-sync-command-payload`
  - `data-skymp-inventory-sync-command-seq`

### Shared protocol ownership

- Shared constants and payload shapes should live with the addon, not in core,
  unless they become cross-addon platform primitives.
- Concrete addon-local naming examples should live with the addon itself.

## New Addon Checklist

Another addon author should be able to build addon `#2` by following this
shape:

1. Pick a stable `addonId`.
2. Define addon-owned message names and packet types with that namespace.
3. Put cross-runtime payload types in an addon-owned shared module.
4. Create a client entry bundle that self-registers with the client host.
5. Create a server entry module that exports `createServerAddon(api, config)`.
6. Keep addon config under `addons.<addonId>` on the server and under
   an addon-owned client scope such as `sp.settings["skymp5-<addonId>"]`.
7. Document any runtime assumptions that are not covered by the public host API.

## Not Yet Formalized

The following are intentionally not promised as stable platform behavior yet:

- addon load ordering guarantees
- client addon auto-discovery conventions
- generic client addon config schema
- addon dependency ordering between addons
- addon sandboxing or isolation guarantees
