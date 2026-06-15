# Skyrim Roleplay Launcher

Desktop launcher for the Skyrim Roleplay SkyMP server. Handles Discord authentication, client file installation, mod management via Mod Organizer, and launching Skyrim through SKSE.
Original by the Frostfall team: https://github.com/F02K/Frostfall-Launcher

Pre-built installers are available at **https://skyrimroleplay.co.uk/**.

## Instructions

---

1) Run build-launcher.bat
2) Check SkyMP/Dist/Launcher for the download
3) nginx is already setup to route api.skyrimroleplay.co.uk/download/SkyrimRoleplayLauncher.exe, change this as needed
4) Rename the launcher to SkyrimRokeplayLauncher.exe so users can download it from the VPS

5) Whenever you edit these files, you need to rebuild it. Also, check the Backend readme.md for more. 



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

## Server lock

If the backend sets `locked: true`, the Play button is disabled for users whose Discord ID is not in `lockedAllowList`. Used during maintenance or testing periods.
