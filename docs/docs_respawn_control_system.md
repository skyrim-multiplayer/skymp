# Custom Respawn Control System

## Overview

The custom respawn control system allows server administrators and modders to implement advanced player death mechanics, such as a "healer/doctor revival" system where dead players cannot auto-respawn until they are revived by another player.

The system is based on the new **`canRespawn`** property, which controls whether a dead player automatically respawns after the configured delay.

## Architecture

### Core Components

1. **`canRespawn` Property** - Boolean property on actors (default: `true`)

   - When `true`: Dead players auto-respawn after the delay configured in `spawnDelay`
   - When `false`: Dead players stay dead until manually revived via `isDead` property

2. **Property Binding** - Access via `mp.set()` / `mp.get()` from TypeScript/gamemode

   ```typescript
   mp.set(actorId, 'canRespawn', false); // Disable auto-respawn
   mp.get(actorId, 'canRespawn'); // Check if respawn enabled
   ```

3. **Death Event Gating** - Modified `DeathEvent.OnFireSuccess()` in C++
   - Checks `actor->GetCanRespawn()` before calling `RespawnWithDelay()`
   - If `canRespawn == false`, the respawn timer is never started

### Implementation Details

**File: `DeathEvent.cpp`**

```cpp
void DeathEvent::OnFireSuccess(WorldState*)
{
  if (actor && actor->GetCanRespawn()) {
    actor->RespawnWithDelay();
  }
};
```

**File: `MpChangeForms.h`**

```cpp
bool canRespawn = true;  // Persisted in ChangeForm
```

**File: `CanRespawnBinding.h/cpp`**

- Property binding to expose `canRespawn` to TypeScript via `mp.set()`/`mp.get()`
- Registered in `PropertyBindingFactory::CreateStandardPropertyBindings()`

## Usage Examples

### Example 1: Basic Healer Revival (TypeScript Gamemode)

```typescript
// When player dies, disable auto-respawn
export const onDeath = (dyingActorId: number, killerId: number) => {
  mp.set(dyingActorId, 'canRespawn', false);
  console.log(`Player ${dyingActorId} is downed. Waiting for healer...`);
};

// Healer casts revival spell
export const revivePlayer = (playerId: number, healerId: number) => {
  // Resurrect the player
  mp.set(playerId, 'isDead', false);

  // Re-enable respawn for next death
  mp.set(playerId, 'canRespawn', true);

  console.log(`Player ${playerId} revived by healer ${healerId}`);
};
```

### Example 2: Delayed Revival with Timeout

```typescript
const downedPlayers = new Map<number, number>(); // playerId -> downTime

export const onDeath = (dyingActorId: number, killerId: number) => {
  mp.set(dyingActorId, 'canRespawn', false);
  downedPlayers.set(dyingActorId, Date.now());

  // Auto-revive after 5 minutes if not revived by healer
  setTimeout(() => {
    if (downedPlayers.has(dyingActorId)) {
      mp.set(dyingActorId, 'isDead', false);
      mp.set(dyingActorId, 'canRespawn', true);
      downedPlayers.delete(dyingActorId);
    }
  }, 300000); // 5 minutes
};

export const healerRevives = (playerId: number) => {
  mp.set(playerId, 'isDead', false);
  mp.set(playerId, 'canRespawn', true);
  downedPlayers.delete(playerId);
};
```

### Example 3: Papyrus-Side Implementation

For Papyrus scripts that need to read/write `canRespawn`:

```papyrus
; In your custom healing spell/power
Function RevivePlayer(Actor akTarget)
  ; Enable respawn
  mp_set(akTarget, "canRespawn", true)

  ; Resurrect
  mp_set(akTarget, "isDead", false)

  ; Optional: Restore health
  akTarget.RestoreActorValue("Health", 100)
EndFunction

; In your death handler
Function HandlePlayerDeath(Actor akPlayer)
  ; Disable auto-respawn
  mp_set(akPlayer, "canRespawn", false)

  ; Wait for healer to revive (or timeout)
  Utility.Wait(300.0)  ; 5 minute timeout

  ; Auto-revive if still downed
  if mp_get(akPlayer, "isDead") as bool
    mp_set(akPlayer, "canRespawn", true)
    mp_set(akPlayer, "isDead", false)
  endif
EndFunction
```

## API Reference

### Property: `canRespawn`

**Type:** `boolean`  
**Default:** `true`  
**Persistence:** Saved in actor's ChangeForm

**Read:**

```typescript
const canRespawn = mp.get(actorId, 'canRespawn');
```

**Write:**

```typescript
mp.set(actorId, 'canRespawn', false); // Disable auto-respawn
mp.set(actorId, 'canRespawn', true); // Enable auto-respawn
```

### Related Properties

- **`isDead`** - Set to `false` to resurrect a dead actor
- **`spawnDelay`** - Configures respawn delay (in seconds, default 25)
- **`spawnPoint`** - Configures respawn location

## Implementation Steps

To implement a healer revival system:

### 1. Disable Respawn on Death

```typescript
export const onDeath = (dyingActorId: number, killerId: number) => {
  mp.set(dyingActorId, 'canRespawn', false);
};
```

### 2. Create Healer Power/Spell

```typescript
export const castHealerRevival = (healerId: number, targetId: number) => {
  const healer = mp.getFormById(healerId);
  const target = mp.getFormById(targetId);

  if (!healer || !target) return;

  // Check distance, PvP restrictions, etc.
  const dx = target.pos[0] - healer.pos[0];
  const dy = target.pos[1] - healer.pos[1];
  const distance = Math.sqrt(dx * dx + dy * dy);

  if (distance > 20) {
    // 20 unit range
    console.log('Too far away to revive');
    return;
  }

  // Revive
  mp.set(targetId, 'isDead', false);
  mp.set(targetId, 'canRespawn', true);
};
```

### 3. (Optional) Add Revival Restrictions

- Check healer profession (e.g., member of "Healers Guild")
- Check cooldown timers
- Require special items (healing herbs, potions)
- Restrict revives in PvP zones

## Behavior Notes

1. **First Death:** `canRespawn` defaults to `true` for new actors
2. **Persistence:** The `canRespawn` state is saved and restored with the actor
3. **Immediate Effect:** Changing `canRespawn` only affects future deaths, not the current death timer
4. **Resurrection Timing:** To revive a downed player, use `mp.set(actorId, "isDead", false)`
5. **Health on Revival:** By default, revived actors have 0% health. Set health explicitly:
   ```typescript
   mp.set(actorId, 'percentages', { health: 0.25, magicka: 1.0, stamina: 1.0 });
   ```

## Backward Compatibility

- Existing servers: `canRespawn` defaults to `true`, so existing respawn behavior is unchanged
- No breaking changes to existing APIs
- Works alongside existing `spawnDelay` and `spawnPoint` properties

## Testing

To test the system:

```typescript
// Simulate player death without respawn
mp.set(playerId, 'isDead', true);
mp.set(playerId, 'canRespawn', false);

// Verify canRespawn is disabled
console.assert(mp.get(playerId, 'canRespawn') === false);

// Revive
mp.set(playerId, 'isDead', false);
mp.set(playerId, 'canRespawn', true);

// Verify player is alive and respawn enabled
console.assert(mp.get(playerId, 'isDead') === false);
console.assert(mp.get(playerId, 'canRespawn') === true);
```

## Performance Impact

- **Minimal:** Only adds a single boolean check in the death event handler
- **No additional memory:** Reuses existing ChangeForm structure
- **No network overhead:** Property bound locally on server

## See Also

- [docs_server_configuration_reference.md](docs_server_configuration_reference.md) - spawn settings
- [docs_serverside_scripting_reference.md](docs_serverside_scripting_reference.md) - mp.set/mp.get API
- [healer-revival-system.ts](../skymp5-server/ts/examples/healer-revival-system.ts) - Complete implementation example
