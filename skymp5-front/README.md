# skymp5-front

This repo contains GUI demo for Skyrim Multiplayer. Original chat interface by **davinchi59** has been ported.

- `npm run build` is used to build the project.
- `npm run watch` is used to start live-reload server (auto-selects a free port).
- `npm run watch:fixed` starts live-reload server on port `1234`.
- `npm run dev:lan` starts live-reload server for LAN/browser testing (`0.0.0.0` + auto port).
- `npm run test:api` runs API integration checks for server list endpoint handling.
- `npm run test:e2e` runs optional Playwright smoke test (if Playwright is installed).

If you start a live-reload server and Skyrim Multiplayer server on the same machine, then live-reload would work in the game.

## How To Use This

Create `config.js` and specify an output folder.

```js
module.exports = {
  /* TIP: Change to '<your_server_path>/data/ui' */
  outputPath: './dist',
};
```

## Dev Server In Browser And Local Network

The webpack dev server is configured to listen on all interfaces.

1. Start frontend dev server:

```bash
npm run watch
```

If port `1234` is already busy, webpack dev server now picks a free port automatically.

To force the classic fixed port behavior:

```bash
npm run watch:fixed
```

For explicit LAN/browser testing with auto-port fallback:

```bash
npm run dev:lan
```

2. Open in browser from the same machine:

```text
http://127.0.0.1:<printed-port>/
```

3. Open from another device in your local network:

```text
http://<your-lan-ip>:<printed-port>/
```

For browser-only UI testing without the game client, you can force the logged-in overlay mode:

```text
http://127.0.0.1:1234/?devUi=1&devOverlay=serverList
http://127.0.0.1:1234/?devUi=1&devOverlay=admin
```

`devOverlay` accepts `serverList`, `admin`, or both comma-separated.

In `?devUi=1` mode, the top dev banner also shows the effective UI URL and current `/api` proxy target.
It includes a lightweight API reachability indicator based on `/api/servers` with clear status (`reachable`, `timeout`, `network error`, or backend/proxy HTTP status), a manual `Retry` button, a "last successful check" age hint, and a "next check in" countdown.
It also provides a `Pause checks`/`Resume checks` toggle (persisted in localStorage), a `Reset warnings` action, and escalates to a warning after repeated consecutive failures.

For stable browser automation selectors, dev banner exposes `data-testid` hooks:

- `dev-banner-health-controls`
- `dev-banner-health-status`
- `dev-banner-retry-btn`
- `dev-banner-pause-btn`
- `dev-banner-reset-warnings-btn`
- `dev-banner-health-meta`
- `dev-banner-health-warning` (only when warning is visible)

You can override host, port, backend target and health-check interval:

```bash
SKYMP_FRONT_HOST=0.0.0.0 SKYMP_FRONT_PORT=1234 SKYMP_FRONT_API_TARGET=http://127.0.0.1:7777 npm run watch
```

```bash
SKYMP_FRONT_HEALTH_MS=5000 npm run watch
```

For E2E smoke checks with Playwright, set the target URL if needed:

```bash
PLAYWRIGHT_E2E_URL=http://127.0.0.1:1234/ npm run test:e2e
```

In browser-only dev overlay mode (`?devUi=1`), frontend metrics posting is suppressed by default if no explicit metrics endpoint is configured. This avoids repeated proxy noise when no local backend is running on port `7777`.

If you want to test metrics ingestion in that mode, expose a backend and set a custom endpoint before loading the page:

```js
window.SKYMP_METRICS_ENDPOINT = 'http://127.0.0.1:7777/api/frontend/metrics';
```
