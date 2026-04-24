# `skymp5-gamemode`

Local gamemode workspace for SkyMP server-side scripting.

## What This Folder Is

This folder is a convenient local place to keep a `gamemode.js` file next to the repo.

In this repo, `skymp5-server` defaults to:

- `./skymp5-gamemode/gamemode.js` if the `skymp5-gamemode` folder exists
- otherwise `./gamemode.js`

You can also override that path with `gamemodePath` in `server-settings.json`.

## Files Here

- `gamemode.js`  
  The active barebones file that the server will load by default.
- `gamemode.example.js`  
  A richer example showing a few common patterns: reload-safe state, event sources, synced properties, and debug helpers.

## Local Build Default

When CMake is configured with `BUILD_GAMEMODE=ON`, this repo now defaults to
building gamemode from the local `skymp5-gamemode/` folder.

The local build path copies:

- `skymp5-gamemode/gamemode.js` to `build/dist/server/gamemode.js`
- the local `skymp5-gamemode/` folder to `build/dist/server/skymp5-gamemode/`

That keeps the dist layout compatible with both:

- `server/gamemode.js`
- the runtime default path of `./skymp5-gamemode/gamemode.js` when the folder exists

## How Loading Works

`skymp5-server` watches the gamemode file and reloads it when it changes.

Practical consequence:

- keep long-lived state on `globalThis` if you want it to survive reloads
- clear timers / intervals you create during reload-sensitive development
- expect server-side hooks like `mp.onDeath` and `mp.onRespawn` to be redefined on each reload

By default, gamemode reloads update server state immediately, but client-side update snippets are not rebroadcast to already connected players unless `enableGamemodeDataUpdatesBroadcast` is enabled in `server-settings.json`.

## Main API Surface

The server exposes a global `mp` object to the gamemode.

Useful documented entry points in this repo:

- `docs/docs_serverside_scripting_reference.md`
- `docs/docs_clientside_scripting_reference.md`
- `docs/docs_properties_system.md`
- `docs/docs_events_system.md`
- `docs/docs_server_configuration_reference.md`

Common patterns from the existing codebase:

- server hooks like `mp.onDeath`, `mp.onRespawn`, `mp.onActivate`
- synced properties via `mp.makeProperty(...)`
- client-generated events via `mp.makeEventSource(...)`
- state reads/writes via `mp.get(...)` and `mp.set(...)`
- Papyrus calls via `mp.callPapyrusFunction(...)`

## Suggested Practices

- Keep `gamemode.js` small while iterating. Move experiments into helpers once they settle down.
- Use a log prefix so hot-reload output stays readable.
- Put reload-sensitive caches or counters on `globalThis`.
- Keep gameplay tunables in `server-settings.json` where possible instead of hardcoding everything.
- Use `private.indexed.*` property names if you plan to query them with `mp.findFormsByPropertyValue(...)`.
- Treat the markdown docs as partial references, not complete coverage of every `mp` capability.

## Good Places To Learn More

- `gamemode.example.js` for a local starter example
- `misc/tests/*.js` for tiny runnable examples of `mp` hooks and property usage
- historical `skymp5-functions-lib/index.ts` for the old in-tree gamemode structure and naming patterns

## Note About Upstream Build Plumbing

Upstream SkyMP still has separate-repo / CI plumbing for `skymp5-gamemode`.

In this branch:

- local gamemode is the default build path
- remote fetch/build remains available by configuring:
  - `BUILD_GAMEMODE=ON`
  - `BUILD_GAMEMODE_FROM_REMOTE=ON`
  - `GITHUB_TOKEN=<token>`

That is different from the local development flow described here. For local experimentation, this folder is enough as long as your server is reading `./skymp5-gamemode/gamemode.js` or an explicit `gamemodePath`.
