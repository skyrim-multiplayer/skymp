# Resource Auto-Download & Distribution

SkyMP implements a FiveM-inspired resource distribution system. The server exposes a public HTTP API that allows clients to discover, verify, and download game resources (scripts and mods) automatically.

## Overview

```
Client connects
      │
      ▼
GET /api/resources/manifest          ← fetch list of all resources + SHA256 hashes
      │
      ▼
Compare hashes against local cache   ← dataDir/cache/resources/
      │
      ├── up-to-date: skip
      │
      └── missing / changed:
              │
              ▼
          GET /api/resources/download/:name   ← stream file, validate X-Resource-Hash
              │
              ▼
          Save to cache, load resource
```

## Endpoints

### `GET /api/resources/manifest`

Public, no authentication required. Returns the complete list of resources available on the server.

**Response (200 OK):**

```json
{
  "version": "1",
  "generatedAt": "2026-04-25T12:00:00.000Z",
  "resources": [
    {
      "name": "SweetPie.esp",
      "kind": "mod",
      "size": 204800,
      "hash": "sha256:e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
    },
    {
      "name": "SweetPieScript.pex",
      "kind": "script",
      "size": 1024,
      "hash": "sha256:abc123..."
    }
  ]
}
```

**Fields:**

| Field | Type | Description |
|-------|------|-------------|
| `version` | string | Manifest schema version (`"1"`) |
| `generatedAt` | ISO 8601 | Server timestamp when manifest was generated |
| `resources[].name` | string | Filename (e.g. `SweetPie.esp`) |
| `resources[].kind` | `"mod"` \| `"script"` | Type of resource |
| `resources[].size` | number | File size in bytes |
| `resources[].hash` | string | `sha256:<hex>` — empty string if file unreadable |

**Resource kinds:**

- `mod` — Elder Scrolls plugin files (`.esm`, `.esp`, `.esl`)
- `script` — Compiled Papyrus scripts (`.pex`)

---

### `GET /api/resources/download/:name`

Public, no authentication required. Streams the raw file bytes for the named resource.

**URL parameter:** `:name` — filename only (no path separators allowed).

**Response headers:**

| Header | Value |
|--------|-------|
| `Content-Type` | `application/octet-stream` |
| `Content-Disposition` | `attachment; filename="<name>"` |
| `X-Resource-Hash` | `sha256:<hex>` — use this to verify the download |

**Error responses:**

| Status | Reason |
|--------|--------|
| `400` | Name contains path separators or is empty |
| `403` | Resolved path escapes allowed resource roots (path traversal attempt) |
| `404` | Resource not listed in manifest or file missing on disk |

---

## Security

### Path Traversal Protection

The download endpoint resolves the requested filename against the same resource roots that `listAdminResources` scans. A request for `../../../etc/passwd` will be rejected with `403` because the resolved path does not fall under any allowed root.

Allowed roots:
- `dataDir` (configured via server settings)
- `./` (server working directory)
- `./scripts`
- `./data`
- `./skymp5-gamemode`

### Hash Verification

Clients **should** verify the SHA256 hash from `X-Resource-Hash` after download. The hash format is `sha256:<lowercase hex>`.

```ts
// Example client-side verification (TypeScript)
import { createHash } from 'crypto';
import { readFileSync } from 'fs';

function verifyResource(filePath: string, expectedHash: string): boolean {
  const content = readFileSync(filePath);
  const actual = 'sha256:' + createHash('sha256').update(content).digest('hex');
  return actual === expectedHash;
}
```

---

## Server Configuration

No additional configuration is required. The manifest is built automatically from the resources already scanned by the admin dashboard (`listAdminResources`). The same `npcSettings` load order and `dataDir` settings apply.

To make a resource available for download, place it in one of the scanned directories:

```
server/
├── data/           ← .esm / .esp / .esl plugins
├── scripts/        ← .pex compiled Papyrus scripts
├── skymp5-gamemode/
└── <dataDir>/      ← as configured in server-settings.json
```

---

## Client Integration (Phase 2 — Planned)

The following describes the intended client-side flow once Phase 2 is implemented in `skymp5-client`:

1. On server connect, fetch `/api/resources/manifest`
2. For each resource in the manifest:
   - Compute path: `<cacheDir>/<serverHost>/<name>`
   - If file exists and hash matches: skip
   - Otherwise: `GET /api/resources/download/<name>`, verify `X-Resource-Hash`, save to cache
3. Load resources from cache before entering the world
4. On disconnect: keep cache (reuse on next connect to same server)

**Cache directory:** `<SkyrimDataDir>/SkyMP/cache/<serverHost_port>/`

---

## Implementation Details

The manifest and download endpoints are registered in `skymp5-server/ts/ui.ts`.

`computeResourceHash(filePath)` reads the file synchronously and returns `sha256:<hex>`. For large files (> a few MB) a streaming hash implementation should be used in Phase 2.

The `listAdminResources(settings, dataDir)` function is shared between the admin `/api/admin/resources` endpoint and the public manifest endpoint to keep the resource discovery logic in one place.

---

## Roadmap

See [ROADMAP_FIVEM_IDEAS.md](../ROADMAP_FIVEM_IDEAS.md) for the phased implementation plan:

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | Done | Server-side manifest + download endpoints |
| 2 | Planned | Admin dashboard resource diagnostics panel |
| 3 | Planned | Client-side manifest fetch, cache, auto-download |
| 4 | Planned | Dependency declarations and load-order in manifest |
