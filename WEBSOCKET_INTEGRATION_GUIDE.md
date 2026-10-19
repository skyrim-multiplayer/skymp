# WebSocket-first Real-Time Updates - Integration Guide

> **Status: ✅ Implementation Complete — 2026-05-17**  
> All integration steps described below have been applied. Backend `adminWebSocket.ts` and frontend `useAdminWebSocket.ts` are live. All 6 push streams are active. WS-first approach is in production with HTTP-polling fallback. This guide is kept as historical reference.

## Overview
This guide shows how to integrate the new WebSocket infrastructure into the existing codebase.

## New Files Created

### 1. Backend: `skymp5-server/ts/adminWebSocket.ts` ✅
- Provides WebSocket server setup and broadcast functions
- Handles admin-updates messaging
- Periodic status broadcasts

### 2. Frontend: `skymp5-front/src/hooks/useAdminWebSocket.ts` ✅
- React hook for WebSocket connection management
- Auto-reconnect with exponential backoff
- Message dispatch system

## Integration Steps

### Step 1: Import in `skymp5-server/ts/ui.ts`

Add to imports (around line 18):
```typescript
import {
  setupAdminWebSocket,
  startAdminUpdateBroadcasts,
  stopAdminUpdateBroadcasts,
  broadcastPlayersUpdate,
  broadcastStatusUpdate,
  broadcastLogEntry,
  broadcastEvent,
  broadcastHistoryEntry,
  broadcastRespawnUpdate,
} from './adminWebSocket';
```

### Step 2: Initialize WebSocket in `main()` function

After `setupLiveConsoleWebSocket(server)` is called (around line 5933), add:
```typescript
setupAdminWebSocket(server);

// Start periodic broadcasts
// This requires access to getOnlinePlayerIds() and server state
const getStatusFn = () => ({
  name: settings.name,
  online: getOnlinePlayerIds().length,
  maxPlayers: settings.maxPlayers,
  uptimeSec: Math.floor((Date.now() - processStartedAt) / 1000),
});

const getPlayersFn = () => {
  // Return current player list in same format as /api/admin/players
  return []; // Implement based on existing player fetching logic
};

startAdminUpdateBroadcasts(getStatusFn, getPlayersFn);
```

### Step 3: Replace Polling Calls in Frontend

Replace polling `setInterval` calls with WebSocket in `adminDashboard/index.tsx`:

**OLD (HTTP Polling):**
```typescript
useEffect(() => {
  const interval = setInterval(async () => {
    const res = await fetch('/api/admin/status');
    const data = await res.json();
    setOnline(data.online);
  }, 1000);
  return () => clearInterval(interval);
}, []);
```

**NEW (WebSocket):**
```typescript
const wsState = useAdminWebSocketMessages('status', (data) => {
  if (data.online !== undefined) {
    setOnline(data.online);
  }
}, visible); // Only listen when visible
```

### Step 4: Broadcast Updates When Actions Occur

When players join/leave, logs are created, etc., call broadcast functions:

**Example: When a player joins:**
```typescript
// In your player join handler
const players = getOnlinePlayerList(); // Get current list
broadcastPlayersUpdate(players);
```

**Example: When a log entry is added:**
```typescript
// In logDashboardLog() or similar
const entry = { ... };
broadcastLogEntry(entry);
```

**Example: When history entry is added:**
```typescript
// In warn/ban/kick handlers
broadcastHistoryEntry(entry);
```

## Migration Path

### Phase 1: Status + Players (High Impact, Low Risk)
1. Wire `useAdminWebSocketMessages('status', ...)` in Players panel
2. Wire `useAdminWebSocketMessages('players', ...)` in status display
3. Keep HTTP polling as fallback for non-critical fields
4. Measure: latency improvement, CPU usage

### Phase 2: Logs + Events (Medium Impact)
1. Add `broadcastLogEntry()` calls when new logs arrive
2. Wire `useAdminWebSocketMessages('log', ...)` in Logs panel
3. Add `broadcastEvent()` calls for admin events
4. Wire `useAdminWebSocketMessages('event', ...)` in Events panel

### Phase 3: History (Append-Only Stream)
1. Add `broadcastHistoryEntry()` calls in warn/ban/kick handlers
2. Wire `useAdminWebSocketMessages('history', ...)` in History panel
3. Prepend new entries to existing list (append-only pattern)

### Phase 4: Respawn (State Updates)
1. Add `broadcastRespawnUpdate()` calls when respawn status changes
2. Wire `useAdminWebSocketMessages('respawn', ...)` in Respawn panel
3. Replace state with latest broadcast data

## Fallback Strategy

The `useAdminWebSocket` hook includes auto-fallback:
- If WebSocket fails to connect → Will keep retrying with exponential backoff
- Network errors are caught and logged
- Frontend continues to function with HTTP polling as backup
- No breaking changes to existing UI

## Performance Targets

After migration:
- **Latency**: 100-500ms → 10-50ms (10x improvement)
- **Polling overhead**: 1 request/sec → 0 requests/sec (when WebSocket active)
- **Server load**: Reduced 50% for admin monitoring
- **Bandwidth**: Reduced significantly (delta updates vs full states)

## Monitoring

Add these metrics to track WebSocket health:
```typescript
// In admin dashboard
const wsMetrics = {
  connectedClients: getAdminWebSocketClientCount(),
  messagesSinceConnect: wsState.messageCount,
  lastUpdateAge: wsState.lastMessageTs
    ? Date.now() - wsState.lastMessageTs
    : null,
  isConnected: wsState.connected,
};
```

## Testing

### Unit Tests
- Test `broadcastAdminUpdate()` sends to all connected clients
- Test message serialization/deserialization
- Test client cleanup on disconnect

### E2E Tests
- Connect WebSocket client and verify connection message
- Trigger server state change (new player) and verify broadcast
- Disconnect and verify auto-reconnect
- Network failure simulation and fallback to polling

### Load Tests
- 50+ concurrent WebSocket connections
- 1000 messages/sec throughput
- Memory usage per active connection (<100KB)

## Rollback Plan

If issues occur:
1. Disable WebSocket by setting `enabled={false}` on hooks
2. Frontend will continue to function with HTTP polling
3. No database changes, no state loss
4. Full backward compatibility maintained

## Status Indicators

Add to admin dashboard diagnostics:
- ✅ WebSocket connected
- 🔄 WebSocket reconnecting (show retry count, next attempt ETA)
- ❌ WebSocket unavailable (fallback to polling)
- 📊 Message rate, latency, connection uptime
