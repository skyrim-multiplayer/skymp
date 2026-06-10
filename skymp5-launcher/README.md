# Skyrim Roleplay Launcher

Desktop launcher for the Skyrim Roleplay SkyMP server. Handles Discord authentication, client file installation, mod management via Vortex, and launching Skyrim through SKSE.
Original by the Frostfall team: https://github.com/F02K/Frostfall-Launcher

Pre-built installers are available at **https://skyrimroleplay.co.uk/**.

---

## Stack

- **Electron 41** — main process (Node.js) + renderer (Chromium)
- **electron-builder** — packaging; produces NSIS installer (Windows), AppImage/deb (Linux), DMG (macOS)
- **electron-store** — persistent JSON config in AppData
- **adm-zip** — ZIP extraction for client file bundles
- **classic-level** — LevelDB reader for Vortex profile state
- Vanilla JS/HTML/CSS — no frontend framework, no build step

---

## Project structure

```
src/
  main.js          Main process: window, IPC handlers, OAuth flow, install, launch
  preload.js       Context-isolated bridge — exposes window.electronAPI to renderer
  config.js        API_URL from env (defaults to https://api.skyrimroleplay.co.uk/)
  vortex.js        Vortex detection (registry), profile reading (LevelDB), mod lookup
  renderer/
    index.html     UI shell: topbar, content grid, modals
    renderer.js    Event listeners, API calls, settings, news/modlist rendering
    styles.css     Dark theme, glass effects, custom fonts
assets/            App icons (.ico, .icns, .png)
```

---

## Development

```bash
npm install
npm start        # or npm run dev
```

Runs with `--dev` flag: DevTools open, loads `.env` from project root.

Copy `.env.example` to `.env` and set `API_URL` if pointing at a local backend:

```
API_URL=http://localhost:4000
```

---

## Building

```bash
npm run build:win    # Windows — NSIS installer (x64)
npm run build:linux  # Linux — AppImage + .deb
npm run build:mac    # macOS — DMG
npm run build        # All platforms
```

Output goes to `dist/`.

---

## How it works

### Startup

1. Main process creates a 1280×720 frameless window and loads `renderer/index.html`.
2. Renderer fetches server status, news, mod list, and launcher version from the backend.
3. Server status polls every 30 seconds.
4. Cached server list and files version are read from electron-store for offline fallback.

### Discord OAuth

1. User clicks **Login with Discord**.
2. Main process starts a temporary local HTTP server on a random port (`127.0.0.1` only).
3. Calls `GET /auth/discord/url?redirect=http://127.0.0.1:<port>/callback` — backend returns the Discord authorization URL.
4. Opens the URL in the system browser (`shell.openExternal`).
5. Discord redirects to the local callback with an authorization code.
6. Main process calls `GET /auth/discord/exchange?code=<code>` — backend exchanges the code for a Discord access token (client secret never leaves the backend).
7. Calls `POST /auth/session` to get a stable `profileId` (offline) or `session` token (online mode).
8. Credentials stored in electron-store: `discordUser`, `discordToken`, `gameProfileId`, `gameSession`.

### Client file installation (direct)

1. `GET /api/files/version` — compare with stored `filesVersion`.
2. If outdated or required files missing: stream `GET /api/files/zip`, extract ZIP into the Skyrim directory.
3. Write `Data/Platform/Plugins/skymp5-client-settings.txt` with server IP/port and auth data.
4. Store new version tag so the next launch skips the download.

Required files checked before launch:
```
Data/Platform/Plugins/skymp5-client.js
Data/SKSE/Plugins/SkyrimPlatform.dll
Data/SKSE/Plugins/MpClientPlugin.dll
```

### Vortex installation

If Vortex integration is enabled:

1. Fetch mod list from backend — find the entry with `collectionSlug`.
2. Open `nxm://skyrimspecialedition/collections/<slug>/revisions/<id>` — Vortex auto-installs the Nexus collection.
3. Download and extract backend files same as the direct path.
4. Write client settings.

### Launch

1. Write client settings with current server IP/port and session data.
2. If Vortex enabled: spawn `Vortex.exe --profile <id> --start-minimized`, wait 5 s for hardlink deployment.
3. Spawn `skse64_loader.exe` from the Skyrim directory. SKSE loads SkyrimPlatform and the SkyMP client plugin, which reads the settings file and connects.

### Client settings file format

Offline mode:
```json
{
  "server-ip": "...",
  "server-port": 7777,
  "master": "",
  "server-master-key": null,
  "gameData": { "profileId": 12345 }
}
```

Online mode:
```json
{
  "server-ip": "...",
  "server-port": 7777,
  "master": "https://api.skyrimroleplay.co.uk/",
  "server-master-key": "<key>",
  "gameData": { "session": "<token>" }
}
```

---

## Persistent store keys

| Key | Type | Purpose |
|-----|------|---------|
| `skyrimPath` | string | Path to Skyrim Special Edition directory |
| `filesVersion` | string | Version tag of installed client files |
| `discordUser` | object | Discord user info for display |
| `discordToken` | string | Discord access token |
| `gameProfileId` | number | Stable player ID (offline mode) |
| `gameSession` | string | Session token (online mode) |
| `vortexPath` | string | Path to Vortex.exe |
| `vortexProfileId` | string | Selected Vortex profile |
| `vortexEnabled` | boolean | Use Vortex for installation |
| `nexusApiKey` | string | Nexus Mods API key |
| `cachedServers` | array | Last-known server list (offline fallback) |

---

## Backend API endpoints used

| Method | Path | Purpose |
|--------|------|---------|
| GET | `/api/servers` | Server list |
| GET | `/api/status` | Online/offline + player count |
| GET | `/api/serverinfo` | Name, max players, lock status, auth config |
| GET | `/api/news` | News cards |
| GET | `/api/modlist` | Mod list with Nexus links |
| GET | `/api/metrics` | Server performance stats |
| GET | `/api/files/version` | Current client files version tag |
| GET | `/api/files/zip` | Client files bundle (ZIP download) |
| GET | `/auth/discord/url` | Discord OAuth authorization URL |
| GET | `/auth/discord/exchange` | Exchange OAuth code for token |
| POST | `/auth/session` | Create/get game session |
| GET | `/api/version` | Launcher update check |

---

## Environment variables

| Variable | Default | Purpose |
|----------|---------|---------|
| `API_URL` | `https://api.skyrimroleplay.co.uk/` | Backend base URL |

In development, set via `.env`. In packaged builds, set as a system environment variable or leave default.

---

## Server lock

If the backend sets `locked: true`, the Play button is disabled for users whose Discord ID is not in `lockedAllowList`. Used during maintenance or testing periods.
