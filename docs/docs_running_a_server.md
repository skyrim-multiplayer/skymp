# Running a Server

Skyrim Multiplayer publishes ready-to-use pre-built server packages on GitHub Releases. **You do not need to download or build the repository to run your own server.**

## Quick Start with Pre-built Releases (recommended)

Pre-built packages are published at:

**<https://github.com/skyrim-multiplayer/skymp/releases>**

### Windows (pre-built)

1. Download `running_server_files_windows_server_dist.zip` from the latest release.
2. Extract the archive to a dedicated folder, e.g. `C:\skymp-server`.
3. Run `launch_server.bat`. It will automatically install Node.js (via `winget`) and npm dependencies on first start.
4. Edit `server-settings.json` to configure your server name, ports, and load order (see [Configuration](#configuration) below).
5. Open the admin dashboard at `http://localhost:8080/admin` (default `uiPort`).

### Linux (pre-built)

1. Download `running_server_files_linux_server_dist.tar.gz` from the latest release.
2. Extract: `tar -xzf running_server_files_linux_server_dist.tar.gz -C /opt/skymp`
3. Run `./launch_server.sh`. It will install Node.js via your package manager and npm dependencies on first start.
4. Edit `server-settings.json` to configure your server.
5. Open the admin dashboard at `http://localhost:8080/admin`.

### Required Skyrim master files (`*.esm`)

Release server archives intentionally do **not** contain Bethesda game master files (`Skyrim.esm`, `Update.esm`, `Dawnguard.esm`, `HearthFires.esm`, `Dragonborn.esm`).

Copy these files from your own legal Skyrim installation into the server data directory root (`data/` by default), then keep the same names in `loadOrder`.

- Typical Windows location: `C:\Program Files (x86)\Steam\steamapps\common\Skyrim Special Edition\Data\`
- Typical Linux/Proton location: `~/.steam/steam/steamapps/common/Skyrim Special Edition/Data/`

Example target layout in server folder:

```text
data/Skyrim.esm
data/Update.esm
data/Dawnguard.esm
data/HearthFires.esm
data/Dragonborn.esm
```

### Updating an existing installation

Place the downloaded archive in the server package root, then run the update helper (preserves `data/` and `server-settings.json`):

**Windows:**
```powershell
.\update_server.ps1 -PackagePath C:\Downloads\running_server_files_windows_server_dist.zip
```

**Linux:**
```sh
./update_server.sh --package /path/to/running_server_files_linux_server_dist.tar.gz
```

Add `-StartAfter` (Windows) or `--start-after` (Linux) to restart the server automatically after the update.

See [Server Update Playbook](docs_server_update_playbook.md) for the full safe-update procedure with backup steps.

---

## Building from Source (advanced / contributors)

If you want to contribute code or need a custom build, follow [docs_cmake_workflow.md](docs_cmake_workflow.md) and [CONTRIBUTING.md](../CONTRIBUTING.md). The compiled server will appear under `build/dist/server` and a convenience launcher is generated as `build/launch_server.bat` / `build/launch_server.sh`.

---

## Installation

### Windows

The server requires `Windows 8.1` / `Windows Server 2012` or higher. It may still launch on older operating systems, but correct work isn't guaranteed.

You obviously need to have 64-bit Windows version since the server is 64-bit program.

You are able to build whole project from sources. Server build would be in `build/dist/server`. Use `launch_server.bat` to launch. A convenience launcher is also generated as `build/launch_server.bat`.

Downloaded server builds should be unpacked into a dedicated server folder. Future updates can be applied with `update_server.ps1` from the server package root while preserving `data` and `server-settings.json`.

If you use `update_server.ps1 -StartAfter`, the script starts the packaged `launch_server.bat`, so the same restart protection and runtime dependency bootstrap are reused after the update.

`launch_server.bat` now includes restart protection: before starting a new server process it checks for an already running `dist_back/skymp5-server.js` process and terminates it automatically.

`launch_server.bat` also validates runtime dependencies on startup: if `node` is missing it attempts automatic installation (via `winget` when available), and if `node_modules` is missing it installs runtime packages automatically.

### Linux

Linux server support is prepared for source builds.

If your build completed successfully, the server launcher will be generated as `build/dist/server/launch_server.sh`. A convenience launcher is also generated as `build/launch_server.sh`.

Downloaded server builds can be updated in place with `update_server.sh --package /path/to/release.tar.gz` from the server package root. The script backs up and preserves `data` and `server-settings.json`.

If you add `--start-after`, the update script starts the packaged `launch_server.sh`, so the same runtime dependency bootstrap path is reused after file replacement.

`launch_server.sh` validates runtime dependencies on startup: if `node` is missing it attempts package-manager based installation, and if `node_modules` is missing it installs runtime packages automatically.

Client support remains Windows-first, but the dedicated server can be prepared for Linux environments.

Ubuntu 24.04 is a good target for production hosting. The current Linux path is best aligned with glibc-based systems such as Ubuntu rather than Alpine or Arch.

## Admin Dashboard

The admin dashboard is available at `http://<host>:<uiPort>/admin`.

## Server Logs

Server output is written both to console and to rotating log files in `logs/` next to the server process.

- File name pattern: `server-<ISO timestamp>.log`
- Retention: files older than 14 days are removed at startup
- Safety cap: only the newest 30 log files are kept at startup

When client-side movement telemetry is enabled, the server also writes periodic structured records as lines starting with `[MovementDebug]` containing JSON payloads (`profileId`, movement interval, extrapolation, correction counters, etc.).

- `uiPort` is taken from `server-settings.json` (if omitted, it falls back to `port`).
- The page uses an in-app login form (not browser Basic Auth popup).
- Credentials are validated against configured admin credentials (secure setup flow), with legacy `adminUiAuth` / `metricsAuth` fallback behavior where applicable.
- Session timeout is 10 minutes of inactivity for dashboard interactions.
- `adminApi.externalUrl` can be left empty so the server auto-detects the external URL from `Host` and `X-Forwarded-Proto` headers.
- Stop/Restart actions in the Admin dashboard are only enabled when a server supervisor is explicitly configured.

### Runtime Readiness API (international support)

For troubleshooting fresh installations, the admin API exposes a runtime readiness report at:

- `GET /api/admin/runtime-readiness`

The response contains language-neutral `code` values and an `overall` state (`ok`, `warn`, `error`) so operators can automate checks regardless of UI language.

It validates common first-start requirements such as:

- Launcher script presence (`launch_server.bat` / `launch_server.sh`)
- `package.json` and server entrypoint availability (`dist_back/skymp5-server.js`)
- `node` and `npm` availability
- `node_modules` bootstrap state
- `release-info.json` parse validity

## Configuration

Once you build the server, you should be able to launch it. But default config values are only usable to verify that server works. After launching the server you will see a server called `My Server` in the master list: https://skymp.io/api/servers.

To make your server reachable from outside your local machine/network, configure bind/listen settings and open required ports in your firewall/router.

```json5
{
  dataDir: 'data',
  loadOrder: [
    'Skyrim.esm',
    'Update.esm',
    'Dawnguard.esm',
    'HearthFires.esm',
    'Dragonborn.esm',
  ],
  listenHost: '0.0.0.0',
  uiListenHost: '0.0.0.0',
  port: 7777,
  uiPort: 8080,
  name: 'My Server',
  adminApi: {
    enabled: true,
    externalUrl: '',
  },
  supervisor: {
    enabled: false,
    stopCommand: '',
    restartCommand: '',
  },
}
```

- You may find out your public IP here http://api.ipify.org
- You need to have ports open. Talk to your Internet provider support if you want to open ports. Status of each port can be checked here https://www.yougetsignal.com/tools/open-ports/. See [Server Ports Usage](docs_server_ports_usage.md) and [Server Configuration Reference](docs_server_configuration_reference.md).
- If you use `LogMeIn Hamachi` or similar software then just type an IP address you got assigned from it. Your friends who share a "local" network with you will be able to connect, players from the Internet will not.

## Ubuntu 24.04 Production Setup

Recommended production pattern on Ubuntu 24.04:

1. Build the server and place it in a stable deployment path such as `/opt/skymp`.
2. Install the systemd service template from [misc/systemd/skymp.service](../misc/systemd/skymp.service).
3. Create a dedicated service user such as `skymp`.
4. Enable and start the service with `systemctl enable --now skymp.service`.
5. Open your game and admin ports in `ufw` or your provider firewall.

Example supervisor configuration for the admin dashboard:

```json5
{
  supervisor: {
    enabled: true,
    stopCommand: '/opt/skymp/misc/systemd/skymp-supervisorctl.sh stop',
    restartCommand: '/opt/skymp/misc/systemd/skymp-supervisorctl.sh restart',
  },
}
```

The helper script template is available at [misc/systemd/skymp-supervisorctl.sh](../misc/systemd/skymp-supervisorctl.sh). For Ubuntu 24.04 you will typically allow the service user to run `systemctl stop skymp.service` and `systemctl restart skymp.service` via `sudoers` without a password prompt.

If you place the admin dashboard behind Nginx or another reverse proxy, make sure it forwards `Host` and `X-Forwarded-Proto` so the server can generate correct external admin URLs automatically.
