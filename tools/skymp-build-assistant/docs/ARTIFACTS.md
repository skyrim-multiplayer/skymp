# SkyMP build artifact inventory

This document maps **producers** (what builds each file) to **outputs** under `build/dist` and **Skyrim install paths** used by sync/install tooling. It is the human-readable companion to [manifest.default.yaml](../manifest.default.yaml).

## Layout rules

- CMake requires the binary directory to be `<repo>/build` (see root [CMakeLists.txt](../../CMakeLists.txt)).
- Most shipped layouts land under `build/dist/client` (game mirror) and `build/dist/server` (Node server).
- VoIP additionally writes `build/dist/voip/*` and copies plugins into `build/dist/client/Data/Platform/Plugins`.

## Client (TypeScript plugin + settings)

| Artifact | Producer | Output path |
|----------|----------|-------------|
| `skymp5-client.js` | CMake target `skymp5-client` -> `yarn build` (webpack) | `build/dist/client/Data/Platform/Plugins/skymp5-client.js` |
| `skymp5-client-settings.txt` | CMake POST_BUILD script `generate_client_settings.cmake` | `build/dist/client/Data/Platform/Plugins/skymp5-client-settings.txt` |

Webpack config: [skymp5-client/webpack.config.js](../../skymp5-client/webpack.config.js). Optional `DEPLOY_PLUGIN=true` copies the plugin to `Data/Platform/PluginsDev` in the game folder.

## Front / in-game UI (React)

| Artifact | Producer | Output path |
|----------|----------|-------------|
| UI bundle(s) | Optional `BUILD_FRONT` -> `yarn build` (webpack) | `build/dist/client/Data/Platform/UI` (via [skymp5-front/config.js](../../skymp5-front/config.js)) |

## Server (Node)

| Artifact | Producer | Output path |
|----------|----------|-------------|
| Bundled server JS | CMake -> `yarn build-ts` (esbuild) | `build/dist/server/dist_back/skymp5-server.js` |
| Launch helpers / configs | CMake install / codegen | `build/dist/server/*` (e.g. `launch_server.bat`, settings - see server CMake) |

Package: [skymp5-server/package.json](../../skymp5-server/package.json).

## VoIP

| Artifact | Producer | Output path |
|----------|----------|-------------|
| Client plugin | `npm run build` / `yarn build` in `skymp5-voip` | `build/dist/client/Data/Platform/Plugins/skymp5-voip.js` |
| Server plugin | same | `build/dist/voip/server/skymp5-voip-server-plugin.js` |
| Dev server | same | `build/dist/voip/voip-dev-server.js` |
| UI + static | esbuild + copy | `build/dist/voip/ui/*` |

Script: [skymp5-voip/scripts/build.js](../../skymp5-voip/scripts/build.js).

## Gamemode (server)

| Artifact | Producer | Output path |
|----------|----------|-------------|
| `gamemode.js` | Default CMake local gamemode build (`BUILD_GAMEMODE=ON`, `BUILD_GAMEMODE_FROM_REMOTE=OFF`) copies `skymp5-gamemode/gamemode.js`; remote mode installs the built output from the separate repo | `build/dist/server/gamemode.js` |
| Nested gamemode entry | Default local CMake gamemode build copies the local `skymp5-gamemode/` folder into server dist | `build/dist/server/skymp5-gamemode/gamemode.js` |

Notes:

- Local gamemode is now the default build path.
- The nested `build/dist/server/skymp5-gamemode/` folder only exists for the local build path.
- Remote gamemode mode still exists behind `BUILD_GAMEMODE_FROM_REMOTE=ON` and `GITHUB_TOKEN`.

## Skyrim Platform native binaries

Built under CMake (Skyrim Platform); typical outputs referenced by [sync-dev-runtime.ps1](../../sync-dev-runtime.ps1):

| Artifact | Output (dist mirror) | Game destination |
|----------|----------------------|------------------|
| `SkyrimPlatform.dll` | `build/dist/client/Data/SKSE/Plugins/SkyrimPlatform.dll` | `<SSE>/Data/SKSE/Plugins/SkyrimPlatform.dll` |
| `SkyrimPlatformImpl.dll` | `build/dist/client/Data/Platform/Distribution/RuntimeDependencies/...` | `<SSE>/Data/Platform/Distribution/RuntimeDependencies/...` |
| `SkyrimPlatformCEF.exe.hidden` | same pattern | same pattern |

Build bin dir (source for sync): `build/skyrim-platform/_platform_se/bin/Release/` (or Debug).

## NirnLab (optional sibling / in-repo checkout)

Paths are resolved like [sync-dev-runtime.ps1](../../sync-dev-runtime.ps1): sibling `../NirnLabUIPlatform` or in-repo `NirnLabUIPlatform`.

| Artifact | Typical source | Game / dist |
|----------|----------------|-------------|
| `NirnLabUIPlugin.dll` | `NirnLabUIPlatform/build/dist/Release/Data/SKSE/Plugins/...` | SKSE Plugins + dist/client mirror |
| NirnLab UI tree | `.../Data/NirnLabUIPlatform` | `<SSE>/Data/NirnLabUIPlatform` + dist |

## Sync / install mechanisms (existing)

1. **[sync-dev-runtime.ps1](../../sync-dev-runtime.ps1)** - SHA256 incremental sync + optional stale VoIP UI file removal; warns on `PluginsDev/skymp5-client.js`.
2. **CMake `INSTALL_CLIENT_DIST`** - `cmake -E copy_directory build/dist/client` -> `SKYRIM_DIR` (full tree copy; see root CMakeLists.txt).
3. **skyrim-platform/tools/dev_service** - packs platform into `build/dist/client` and may `copySync` to the game folder when configured.

The **SkyMP Build Assistant** (`skymp-dev`) unifies visibility and incremental sync using the same hash model as `sync-dev-runtime.ps1`, driven by [manifest.default.yaml](../manifest.default.yaml).
