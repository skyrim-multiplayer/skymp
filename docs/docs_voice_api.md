# Voice Activity Detection API

## Overview

The Voice Activity Detection (VAD) API is a **provider-agnostic interface** for voice activity synchronization in SkyMP. It enables any voice system (YACA-TS, Discord, Mumble, custom VoIP, etc.) to integrate with the SkyMP server and provide synchronized lip-sync animations across clients.

**Key Goals:**
- **Provider abstraction**: Core server doesn't depend on any specific voice system
- **Adapter pattern**: Each voice provider implements a lightweight adapter
- **Proximity-aware broadcasting**: Voice state sent only to nearby players
- **Health monitoring**: Admin visibility into voice pipeline health

---

## Architecture

```
Voice Provider                Server-Side                       Client-Side
(TeamSpeak, Discord, etc.)    (SkyMP)                          (Skyrim)
        |                           |                              |
        |-- Voice Event ----------->| (Custom Packet)              |
        |                           |                              |
        |    ┌─────────────────────┐|                              |
        |    | Voice Activity API   ||                              |
        |    | Manager             |<--- Broadcast VAD State ----->| (Animation Sync)
        |    │                     ||                              |
        |    │ - State Registry    ||   - Proximity Filter         |
        |    │ - Inactivity Timer  ||   - Client Notification      |
        |    │ - Health Tracking   ||                              |
        |    └─────────────────────┘|                              |
        |                           |                              |
        |      [Provider Adapter]   |                              |
        |    (Translates to VAD)    |                              |
        |                           |                              |
```

---

## Core Interfaces

### VoiceActivityState

Standard state structure that all providers must emit:

```typescript
interface VoiceActivityState {
  actorId: string;              // Skyrim actor ID
  isSpeaking: boolean;          // Is player currently speaking?
  voiceRange: number;           // Proximity tier in meters
  providerId?: string;          // Optional: "yaca-ts", "discord-bot", etc.
  timestamp: number;            // When state was captured (ms)
  metadata?: Record<string, any>; // Optional provider-specific data
}
```

### IVoiceProviderAdapter

Interface that every voice provider must implement:

```typescript
interface IVoiceProviderAdapter {
  readonly providerId: string;
  readonly isReady: boolean;
  
  initialize(): Promise<void>;
  shutdown(): Promise<void>;
  onVoiceActivityUpdate(callback: (state: VoiceActivityState) => void): void;
  getHealthStatus?(): { isHealthy: boolean; uptime: number; ... };
}
```

### VoiceActivityConfig

Server-side configuration:

```typescript
interface VoiceActivityConfig {
  enabled: boolean;              // Global VAD enable/disable
  voiceRangeTiers: number[] | null;  // Supported voice ranges
  defaultVoiceRange: number;     // Fallback range if not specified
  proximityDistance: number;     // Broadcast range to clients
  inactivityTimeoutMs: number;   // Auto-clear speaking state after N ms
  maxConcurrentSpeakersPerCell?: number;
  allowedProviders?: string[] | null; // Provider whitelist
}
```

---

## Implementation Steps

### 1. Initialize Voice Activity Manager

In `skymp5-server/ts/index.ts` (server startup):

```typescript
import { createVoiceActivityManager } from "./voiceActivityManager";
import { createYacaTeamSpeakAdapter } from "./adapters/YacaTeamSpeakAdapter";

// Initialize manager (if VAD is enabled in config)
if (serverSettings.voiceActivity?.enabled) {
  const voiceManager = createVoiceActivityManager(serverSettings.voiceActivity);
  
  // Set broadcast callback to send voice state to clients
  voiceManager.setBroadcastCallback(async (state) => {
    // Broadcast custom packet to clients
    // Pattern: mp.syncVoiceActivity(state) or similar
    await broadcastPacket("syncVoiceActivity", state);
  });
  
  // Register YACA adapter
  const yacaAdapter = createYacaTeamSpeakAdapter();
  await voiceManager.registerAdapter(yacaAdapter);
  
  // Store reference for later use
  globalThis.voiceActivityManager = voiceManager;
}
```

### 2. Register Custom Packet Handler for YACA Events

In gamemode script (TypeScript):

```typescript
// When YacaBridgeQuest sends voice activity event
mp.addEventListener("customPacket", (p) => {
  if (p.eventName === "yaca:voiceActivity") {
    const adapter = globalThis.voiceActivityManager?.adapters.get("yaca-ts");
    if (adapter instanceof YacaTeamSpeakAdapter) {
      adapter.handleYacaVoiceEvent(p.actorId, p.isSpeaking, p.voiceRange);
    }
  }
});
```

### 3. Implement Client-Side Animation Sync

Client receives broadcast packet and applies lip-sync animation:

```typescript
// Client-side (Papyrus or SkyrimPlatform)
on("syncVoiceActivity", (state: VoiceActivityState) => {
  const actor = Game.getFormFromUniqueID(state.actorId) as Actor;
  if (!actor) return;
  
  if (state.isSpeaking) {
    // Start lip-sync animation
    actor.playIdle(Game.getForm(0x0005B1B0)); // IdleDialogueLock
  } else {
    // Stop animation
    actor.stopTranslation();
  }
});
```

### 4. Extend YacaBridgeQuest to Send Events

In `SkyMP-YACA-TS/skymp5-scripts/psc/YacaBridgeQuest.psc`:

```papyrus
; When YACA plugin detects voice activity
Event OnVoiceActivityChanged(string playerId, bool isSpeaking, float voiceRange)
  ; Send to server
  int handle = ModAPI.GetObjectReference("mp")
  if (handle > 0)
    ModAPI.CallObjectMethod(handle, "callGamemodeApi", ...
      {"method": "yaca:voiceActivity", \
       "playerId": playerId, \
       "isSpeaking": isSpeaking, \
       "voiceRange": voiceRange})
  endif
EndEvent
```

---

## Admin API Extensions

### GET /api/admin/voice/status

Get overall VAD system health:

```json
{
  "isEnabled": true,
  "activeSpeakers": 3,
  "registeredAdapters": [
    {"providerId": "yaca-ts", "isHealthy": true, "uptime": 3600000},
    {"providerId": "discord-bot", "isHealthy": false, "uptime": 1800000}
  ],
  "lastBroadcastTime": 1715691234567,
  "errors": []
}
```

### GET /api/admin/voice/speakers

Get currently active speakers:

```json
[
  {
    "actorId": "0x0001A69E",
    "actorName": "Jervar",
    "isSpeaking": true,
    "voiceRange": 8.0,
    "providerId": "yaca-ts",
    "timestamp": 1715691234567
  }
]
```

### POST /api/admin/voice/range

Dynamically adjust player voice range:

```json
{
  "actorId": "0x0001A69E",
  "voiceRange": 15.0
}
```

---

## Creating a Custom Voice Adapter

To integrate a new voice system, implement `IVoiceProviderAdapter`:

### Example: Discord Voice Adapter

```typescript
export class DiscordVoiceAdapter implements IVoiceProviderAdapter {
  readonly providerId = "discord-bot";
  private voiceActivityCallbacks: Set<(state: VoiceActivityState) => void> = new Set();
  
  async initialize(): Promise<void> {
    // Connect to Discord bot API
    // Subscribe to voice state update events
  }
  
  async shutdown(): Promise<void> {
    // Disconnect from Discord
  }
  
  onVoiceActivityUpdate(callback: (state: VoiceActivityState) => void): void {
    this.voiceActivityCallbacks.add(callback);
  }
  
  // Called when Discord bot detects user joined/left voice channel
  handleDiscordVoiceEvent(userId: string, isInVoiceChannel: boolean): void {
    const state: VoiceActivityState = {
      actorId: this.discordUserToActorId(userId), // Map Discord ID to Skyrim actor
      isSpeaking: isInVoiceChannel,
      voiceRange: 8.0, // Default range
      providerId: this.providerId,
      timestamp: Date.now(),
    };
    
    for (const callback of this.voiceActivityCallbacks) {
      callback(state);
    }
  }
  
  // ... implement other interface methods
}
```

---

## Server Configuration

Add to `server-settings.json`:

```json
{
  "voiceActivity": {
    "enabled": true,
    "providers": ["yaca-ts"],
    "voiceRangeTiers": [1.0, 3.0, 8.0, 15.0, 20.0, 25.0, 30.0, 40.0],
    "defaultVoiceRange": 8.0,
    "proximityDistance": 100.0,
    "inactivityTimeoutMs": 1000,
    "maxConcurrentSpeakersPerCell": 50
  }
}
```

---

## Troubleshooting

### No voice activity events reaching server
- Check YacaBridgeQuest is loaded (verify in Papyrus logs)
- Verify custom packet handler is registered in gamemode
- Enable debug logging in Voice Activity Manager

### Lip-sync animations not playing on clients
- Verify clients receive `syncVoiceActivity` broadcasts
- Check animation form IDs (IdleDialogueLock, etc.)
- Ensure client-side Papyrus/SkyrimPlatform event handlers are active

### Provider adapter crashes
- Check adapter logs: `Admin API /api/admin/voice/status` → `errors[]`
- Verify provider initialization parameters in server config
- Test adapter in isolation

### High latency in voice state updates
- Check proximity filter isn't excluding too many clients
- Monitor `lastBroadcastTime` in admin status
- Reduce `inactivityTimeoutMs` if animation stutter occurs

---

## References

- YACA-TS Reference: `D:/GitHub/SkyMP-YACA-TS/README.md`
- Adapter Implementation: `skymp5-server/ts/adapters/YacaTeamSpeakAdapter.ts`
- API Types: `skymp5-server/ts/voiceActivityApi.ts`
- Manager: `skymp5-server/ts/voiceActivityManager.ts`
