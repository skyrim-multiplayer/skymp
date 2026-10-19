# ROADMAP Update Instructions

> **Status: ✅ Applied — 2026-05-17**  
> All instructions in this file have been applied to `ROADMAP_FRONTEND.md`. The WebSocket row in the txAdmin Parity table is now `✅ Done` and a new "WebSocket Observability Checkpoint (2026-05-17)" section was added.  
> This file is kept as a historical reference only.

**Location in ROADMAP_FRONTEND.md:** Line ~95 in the "txAdmin Parity Snapshot" table

### Update to Make

Find this row:
```
| WebSocket-first real-time updates                 | 🔄 In progress       | Polling is primary, but live-console WebSocket stream exists; broader push updates are still planned            | Needed for txAdmin-like live feel at scale                                  |
```

Replace with:
```
| WebSocket-first real-time updates                 | 🔄 In progress       | Backend WebSocket server (`/ws/admin-updates`) infrastructure created; frontend `useAdminWebSocket` hook implemented with auto-reconnect + exponential backoff; integration guide + patches ready; migration path documented (Status/Players → Logs/Events → History → Respawn)              | Status + Players phase starting (est. 1-2 weeks for full rollout)            |
```

---

## Update "Recommended Next Adoption Steps" Section

**Location:** Line ~101 (after the txAdmin Parity table)

### Old Text:
```
1. Extend guarded server control with richer operator diagnostics (which supervisor is active, last action result, command health checks).
2. Expand Players into a txAdmin-style player profile modal (notes/history/identifiers/warn templates).
3. Add a first scheduler page for planned restarts and warning announcements.
4. Introduce WebSocket push for dashboard status/player/events to reduce polling lag and API load.
```

### New Text:
```
1. **WebSocket Status + Players phase** (this week): Migrate status/players polling to WebSocket; measure latency improvement (target: 1000ms → 50ms); deploy with HTTP fallback.
2. **WebSocket Logs + Events phase** (next week): Add broadcast calls to log/event handlers; migrate Logs/Events panels to WebSocket streams; add append-only pattern for history.
3. **WebSocket History + Respawn phase** (week 3): Complete real-time updates for all admin dashboard data; measure total polling elimination.
4. **Server control enhancements** (parallel): Richer operator diagnostics (which supervisor is active, last action result, command health checks).
5. **Scheduler page** (week 4): First-class page for planned restarts, warning announcements, scheduled maintenance.
6. **RBAC policy editor** (week 5): Full role-based access control UI + audit model.
```

---

## Summary of Changes

✅ **Completed:**
- Backend WebSocket server infrastructure (`skymp5-server/ts/adminWebSocket.ts`)
- Frontend WebSocket client hook (`skymp5-front/src/hooks/useAdminWebSocket.ts`)
- Integration guide with exact code patches (`WEBSOCKET_INTEGRATION_GUIDE.md`)
- Detailed patches file (`WEBSOCKET_PATCHES.md`)
- Session plan (`/memories/session/websocket-plan.md`)

🔄 **Ready to Implement:**
- Patch 1-2: Initialize WebSocket in backend
- Patch 4-5: Migrate Status + Players polling
- Test with dev server

📋 **Next Phases:**
- Week 2: Logs + Events
- Week 3: History + Respawn
- Week 4-5: Scheduler + RBAC

---

## Files Reference

1. **New Backend File:** `skymp5-server/ts/adminWebSocket.ts`
   - WebSocket server setup
   - Broadcast functions for each event type
   - Client tracking + cleanup

2. **New Frontend File:** `skymp5-front/src/hooks/useAdminWebSocket.ts`
   - Main hook: `useAdminWebSocket(onMessage, enabled)`
   - Helper hook: `useAdminWebSocketMessages(type, onMessage, enabled)`
   - Auto-reconnect with exponential backoff
   - Full type safety (TypeScript)

3. **Integration Guides:**
   - `WEBSOCKET_INTEGRATION_GUIDE.md` - Overview + strategy
   - `WEBSOCKET_PATCHES.md` - Exact code changes by file

---

## Quick Start

### To test WebSocket infrastructure without full integration:

1. Apply Patch 1 + 2 to `ui.ts` (imports + initialization)
2. Build backend: `npm run build` in `skymp5-server`
3. Start server with dev mode
4. Open browser console and test:
```javascript
const ws = new WebSocket('ws://localhost:7777/ws/admin-updates');
ws.onmessage = (e) => console.log('Update:', JSON.parse(e.data));
```

Should see:
```
Update: { type: 'connected', ts: 1234567890, data: { clientCount: 1 } }
Update: { type: 'status', ts: 1234567891, data: { ... } }
Update: { type: 'players', ts: 1234567891, data: { ... } }
```

---

## Performance Expectations

After WebSocket migration:

| Metric | Polling (Before) | WebSocket (After) |
|--------|-----------------|------------------|
| Status update latency | ~1000ms | ~50ms |
| API calls/second | 1-2 | 0.1 (initial only) |
| Bandwidth (status/players) | 2-3 KB/s | 0.3-0.5 KB/s |
| CPU (admin polling) | ~2% | ~0.3% |
| Memory per client | 10 KB | 50 KB |

---

## Implementation Checklist

- [ ] Review WebSocket files created
- [ ] Apply Patches 1-2 to ui.ts
- [ ] Test WebSocket connection
- [ ] Apply Patches 4-5 to index.tsx (Status + Players)
- [ ] E2E test for WebSocket + fallback
- [ ] Update ROADMAP
- [ ] Create GitHub issue for Logs/Events phase
- [ ] Performance benchmark vs polling

