# SkyMP Build Assistant (`skymp-dev`)

Local supervisor for SkyMP development: **manifest-driven artifact sync** (SHA256, same idea as [`sync-dev-runtime.ps1`](../../sync-dev-runtime.ps1)), **build profile orchestration**, **`doctor` checks**, **`watch`**, and a **localhost dashboard**.

## Setup

From the repository root:

```bash
cd tools/skymp-build-assistant
npm install
npm run build:ui
```

Optionally link the CLI globally:

```bash
npm link
```

Or run via `npx` / `node` (see below).

## Dashboard UX

The dashboard is now a small frontend app with tabs for:

- `Overview`
- `Builds`
- `Mods`
- `Sync`
- `Doctor`
- `Settings`

The `Settings` tab is the primary place to manage local configuration. It reads and writes **`<repo>/.skymp-dev.yaml`** automatically.

The `Builds` tab is the Phase 1 control plane. It lists named build profiles, shows prerequisites and disabled reasons, and lets you launch or cancel builds while polling live-ish logs and recent history from the dashboard API.

Phase 2 adds runtime supervision to the dashboard as well: the `Overview` and `Builds` views now expose a compact runtime control panel for launching Skyrim through SKSE, watching running state, and force-stopping the game when needed.

The dashboard also includes a `Mods` tab that inventories the current Skyrim `Data` directory, shows the resolved `Plugins.txt` state, and lets you enable or disable plugin-backed mods. Non-plugin entries such as loose folder groups, SKSE DLLs, and Skyrim Platform JS plugins are surfaced as inventory-only for now.

## Configuration

You can still create the file manually from [`examples/skymp-dev.example.yaml`](examples/skymp-dev.example.yaml), but it is no longer required for first-time setup.

The current settings fields are:

- `skyrimRoot`
- `skseLoaderPath`
- `pluginsTxtPath`
- `buildDir`
- `nirnLabOutputDir`

If `skyrimRoot` is not configured, the dashboard will still run, but game-target sync entries, runtime launch controls, and the Mods tab inventory will be marked as needing configuration.

## Commands

Run from anywhere; the tool auto-detects the repo by walking upward for `tools/skymp-build-assistant/manifest.default.yaml`.

| Command | Description |
|--------|-------------|
| `skymp-dev doctor` | Submodule / build dir / Skyrim path / SKSE runtime footprint / Plugins.txt / PluginsDev warnings |
| `skymp-dev status` | Compare each manifest source-to-destination pair (dirty / ok / skipped) |
| `skymp-dev sync` | Incremental copy (use `--dry-run` to preview) |
| `skymp-dev sync --only <artifactId>` | Sync a single artifact block from the manifest |
| `skymp-dev runtime status` | Show whether Skyrim is ready to launch, already running, or blocked by config/runtime issues |
| `skymp-dev runtime launch` | Launch Skyrim via the resolved `skse64_loader.exe` |
| `skymp-dev runtime stop` | Force-stop any running `SkyrimSE.exe` process |
| `skymp-dev build profiles` | List the Phase 1 build profiles and their current prerequisites |
| `skymp-dev build run <profileId>` | Run one named build profile and stream its logs |
| `skymp-dev build run clean-build-dir --yes` | Confirm and run a destructive clean of the configured build directory |
| `skymp-dev build run rebuild-all --yes` | Confirm a clean rebuild of the core local workflows |
| `skymp-dev build jobs` | Show the active job plus recent build history |
| `skymp-dev build cancel [jobId]` | Cancel the active or queued build job |
| `skymp-dev watch` | Debounced re-sync when `build/dist` / platform binaries / gamemode change |
| `skymp-dev dashboard` | **http://127.0.0.1:8790** - tabbed UI + `/api/meta`, `/api/status`, `/api/doctor`, `/api/config`, `/api/runtime`, `/api/mods`, `/api/build/*`, `POST /api/sync` |
| `skymp-dev dashboard-stop` | Stop the running dashboard server, or inspect the dashboard port if state is missing |

### Examples

```bash
node tools/skymp-build-assistant/bin/skymp-dev.js doctor
node tools/skymp-build-assistant/bin/skymp-dev.js status --json
node tools/skymp-build-assistant/bin/skymp-dev.js sync --dry-run
node tools/skymp-build-assistant/bin/skymp-dev.js runtime status --json
node tools/skymp-build-assistant/bin/skymp-dev.js runtime launch
node tools/skymp-build-assistant/bin/skymp-dev.js runtime stop
node tools/skymp-build-assistant/bin/skymp-dev.js build profiles
node tools/skymp-build-assistant/bin/skymp-dev.js build run build-assistant-ui
node tools/skymp-build-assistant/bin/skymp-dev.js build run build-all
node tools/skymp-build-assistant/bin/skymp-dev.js build run rebuild-all --yes
node tools/skymp-build-assistant/bin/skymp-dev.js build jobs --json
node tools/skymp-build-assistant/bin/skymp-dev.js build cancel
node tools/skymp-build-assistant/bin/skymp-dev.js watch
node tools/skymp-build-assistant/bin/skymp-dev.js dashboard --port 8790
node tools/skymp-build-assistant/bin/skymp-dev.js dashboard --dev --port 8790
node tools/skymp-build-assistant/bin/skymp-dev.js dashboard-stop
node tools/skymp-build-assistant/bin/skymp-dev.js dashboard-stop --port 8790 --force
```

Explicit repo root:

```bash
node tools/skymp-build-assistant/bin/skymp-dev.js --repo "C:/src/skymp" doctor
```

## Manifest format

- Default manifest: [`manifest.default.yaml`](manifest.default.yaml)
- JSON Schema: [`schema/manifest.schema.json`](schema/manifest.schema.json)
- Human inventory of producers/paths: [`docs/ARTIFACTS.md`](docs/ARTIFACTS.md)

Artifacts list **sources** and **destinations** with `${variable}` placeholders expanded from config + built-in context (`repoRoot`, `buildDir`, `skyrimPlatformDir`, `nirnLabOutputDir`, ...).

## UI Development

Run the frontend dev server:

```bash
npm run dev:ui
```

Or run the dashboard itself in dev mode with hot reload for React and CSS changes:

```bash
node tools/skymp-build-assistant/bin/skymp-dev.js dashboard --dev
```

Build the dashboard frontend for the Express server:

```bash
npm run build:ui
```

The dashboard server serves built assets from `tools/skymp-build-assistant/ui/dist`.

## Build Profiles

Phase 1 ships the following profile families:

- Workflow bundles: `build-all`, `rebuild-all`, `clean-build-dir`
- CMake: `configure-cmake`, `reset-cmake-cache`, `cmake-default`, `cmake-client-only`, `cmake-server-only`, `cmake-platform-only`
- In-tree package builds: `front-in-tree-build`, `voip-build`
- Assistant build: `build-assistant-ui`
- Conditional advanced profiles: `cmake-front-local`, `cmake-gamemode`, `cmake-nirnlab`, `ctest`

Profiles expose prerequisites and disabled reasons through both the CLI and dashboard. That includes repo/env requirements such as the local `skymp5-front` checkout, the local `skymp5-gamemode/gamemode.js` entry file for the default gamemode build, or `GITHUB_TOKEN` when `BUILD_GAMEMODE_FROM_REMOTE=ON`.

Destructive profiles such as `clean-build-dir`, `rebuild-all`, and `reset-cmake-cache` require explicit confirmation. Use `--yes` in the CLI, and confirm the warning prompt in the dashboard before the job is submitted.

### Visual Studio Developer PowerShell

For CMake-based profiles on Windows, start `skymp-dev` from **Developer PowerShell for VS 2022** (or another shell where the Visual Studio toolchain and `cmake` are already on `PATH`).

If you launch the dashboard or CLI from a plain shell that does not have that environment, the assistant will now mark those CMake profiles as blocked instead of trying to run them and failing halfway through.

## Runtime Supervision

Runtime supervision is intentionally narrow in this phase:

- Launch Skyrim through `skse64_loader.exe`
- Detect whether `SkyrimSE.exe` is already running
- Force-stop the game from either the CLI or dashboard
- Surface lightweight doctor warnings for missing SKSE / Skyrim Platform / SkyMP runtime footprint under the configured game install

This phase does **not** inspect `plugins.txt`, `loadorder.txt`, MO2, or Vortex state yet.

## Mods Inventory

The first Mods tab release is intentionally conservative:

- Inventory the current Skyrim `Data` directory using best-effort grouping
- Toggle plugin-backed mods (`.esp`, `.esm`, `.esl`) through `Plugins.txt`
- Show non-plugin content such as top-level folder groups, SKSE DLLs, and Skyrim Platform JS plugins as inventory-only
- Block plugin toggles while Skyrim is running, so `Plugins.txt` changes do not race a live session

This does **not** add MO2/Vortex integration, file-moving disable logic, installs, downloads, or a full load-order editor yet.

## Relation to existing scripts

- **`sync-dev-runtime.ps1`** remains the canonical PowerShell workflow; `skymp-dev` mirrors its mapping in YAML and is easier to extend (extra artifacts, dashboard, CI integration).
- CMake **`INSTALL_CLIENT_DIST`** does a full `copy_directory` - different semantics; use when you want to mirror the entire `dist/client` tree.
