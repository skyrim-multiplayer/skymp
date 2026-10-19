# FiveM-Inspired Roadmap for SkyMP

This roadmap translates key architectural concepts and best practices from FiveM (citizenfx/fivem) into actionable tasks for SkyMP, focusing on stability, performance, player synchronization, and resource/mod download automation.

---

## Completed & Implemented Patterns

### Property Binding Architecture

- **Implementation:** `canRespawn` property binding system for custom server-side control
- **Pattern:** Read-write property bindings exposed via `mp.set()`/`mp.get()` from TypeScript
- **Benefit:** Clean separation of concerns; enables advanced game mechanics (healer revival systems) without C++ monoliths
- **Lesson:** FiveM-style flexible property systems enable complex gameplay without architectural bloat
- **Related Files:** CanRespawnBinding.h/cpp, IsDeadBinding.cpp, SpawnDelayBinding.cpp (reference architecture)

### Configuration-Driven Gameplay

- **Implementation:** `startSpawn`, `starterInventory` configurable via server-settings.json
- **Pattern:** Settings interface -> CMake generation -> Runtime property binding
- **Benefit:** Non-technical server admins can customize spawn points and starter gear without code changes
- **Related Files:** settings.ts, spawn.ts, generate_server_settings.cmake

### Startup Process Hardening

- **Implementation:** Windows launchers now auto-terminate stale `dist_back/skymp5-server.js` processes before startup
- **Pattern:** Supervisor-like preflight process cleanup in launcher scripts
- **Benefit:** Avoids startup collisions and peer init failures caused by orphaned background node instances
- **Related Files:** `skymp5-server/CMakeLists.txt`, `build/dist/server/launch_server.bat`

---

## 1. Resource Distribution API (Phase 1 - Done)

**Goal:** Public HTTP endpoints so clients can fetch the server's resource manifest and download individual files, forming the foundation for FiveM-style auto-download.

- **Status:** Implemented -- `GET /api/resources/manifest` and `GET /api/resources/download/:name` in `skymp5-server/ts/ui.ts`
- **Manifest response:** JSON with `version`, `generatedAt`, and per-resource `name/kind/size/sha256 hash`
- **Download endpoint:** Streams the file after path-traversal validation; sets `X-Resource-Hash` header
- **Security:** Path restricted to known resource roots (`dataDir`, `./scripts`, `./data`, `./skymp5-gamemode`)
- **See:** `docs/docs_resource_auto_download.md` for full architecture

---

## 2. Resource Diagnostics & Validation Panel

**Goal:** Wire the manifest data into the admin dashboard with a dedicated panel: show per-resource hash, size, kind; highlight missing or corrupt files; allow manual refresh.

- **Effort:** Medium
- **Risk:** Low (isolated, non-intrusive)
- **Affected Files:** skymp5-front admin dashboard, `ui.ts`

---

## 3. Client-Side Resource Sync (Phase 2)

**Goal:** On connect, the client fetches `/api/resources/manifest`, compares hashes with its local cache (`dataDir/cache/resources/`), downloads missing or changed files, and loads them before entering the world.

- **Effort:** High
- **Risk:** Medium (requires changes in `skymp5-client`)
- **Affected Files:** `skymp5-client/src/services/services/remoteServer.ts`, cache manager, resource loader

---

## 4. Resource Manifest & Dependency System (Phase 3)

**Goal:** Extend the manifest with explicit dependency declarations, semantic versioning, and load-order constraints. Enables operators to declare `"requires": ["Skyrim.esm"]` style rules.

- **Effort:** Medium
- **Risk:** Medium (touches resource loading)
- **Affected Files:** `manifestGen.ts`, dataDir, resource loader scripts

---

## 5. State Bag System (Key-Value Sync)

**Goal:** Implement a lightweight, extensible state bag system for per-entity, per-player, and global key-value sync, inspired by FiveM's state bags. Enables efficient, granular sync and reduces heavy entity replication.

- **Effort:** High
- **Risk:** Medium-High (core sync changes)
- **Affected Files:** skymp5-server, skymp5-client, entity sync modules

---

## 6. Entity Relevancy & Ownership Migration

**Goal:** Introduce entity relevancy/culling and explicit ownership migration for efficient network sync and server authority, inspired by FiveM's entity scoping and migration.

- **Effort:** High
- **Risk:** High (core sync/ownership logic)
- **Affected Files:** skymp5-server, skymp5-client, entity sync modules

---

## Prioritization & Next Steps

1. **Phase 1 done** -- Public manifest + download endpoints live in `ui.ts`; see `docs/docs_resource_auto_download.md`
2. **Next** -- Admin dashboard panel for resource diagnostics (item 2, low risk)
3. **Phase 2** -- Client-side manifest fetch, cache, and download on connect (item 3)
4. **Phase 3** -- Dependency declarations and load-order in manifest (item 4)
5. **Later** -- State Bag System and Entity Relevancy/Ownership

---

_This roadmap is based on a deep analysis of FiveM's architecture and tailored to SkyMP's current pain points and future goals. Each task can be further broken down into subtasks as needed._
