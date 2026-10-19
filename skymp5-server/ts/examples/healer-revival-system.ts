/**
 * Healer/Doctor Revival System Example
 *
 * This script demonstrates how to use the new canRespawn property
 * to implement a custom healing/revival system where:
 * - Dead players cannot auto-respawn
 * - Only a designated healer can revive them
 * - The healer uses a power/ability to revive
 */
import { MpActor } from 'skymp/script-objects';

// Configuration
const HEALER_POWER_ID = 0x12345678; // Replace with actual power ID
const MAX_DOWNED_TIME = 300000; // 5 minutes in milliseconds

// Track downed players
const downedPlayers = new Map<number, { downedAt: number; healer?: number }>();

// Listen to death events
export const onDeath = (dyingActorId: number, killerId: number) => {
  const actor = mp.getFormById(dyingActorId) as MpActor;

  if (!actor || !actor.isPlayer) return;

  // Disable auto-respawn for this player
  mp.set(dyingActorId, 'canRespawn', false);

  // Track that player is downed
  downedPlayers.set(dyingActorId, { downedAt: Date.now() });

  // Optional: Send message to player
  console.log(
    `Player ${dyingActorId} is downed. Waiting for healer to revive...`,
  );

  // Optional: Auto-revive after 5 minutes if no healer arrives
  setTimeout(() => {
    if (downedPlayers.has(dyingActorId)) {
      revivePlayer(dyingActorId, undefined, true);
    }
  }, MAX_DOWNED_TIME);
};

// Healer uses a power to revive nearby downed player
export const onPowerCast = (healerId: number, powerId: number) => {
  if (powerId !== HEALER_POWER_ID) return;

  const healer = mp.getFormById(healerId) as MpActor;
  if (!healer) return;

  // Find nearest downed player within ~20 units
  const nearestDowned = findNearestDownedPlayer(healer.pos, 20);
  if (nearestDowned) {
    revivePlayer(nearestDowned.id, healerId, false);
  }
};

/**
 * Revive a downed player
 * @param playerId - ID of player to revive
 * @param healerId - ID of healer (optional)
 * @param isAutoRevive - Whether this is an auto-revive after timeout
 */
function revivePlayer(
  playerId: number,
  healerId?: number,
  isAutoRevive = false,
) {
  if (!downedPlayers.has(playerId)) return;

  const actor = mp.getFormById(playerId) as MpActor;
  if (!actor) return;

  // Resurrect the player
  mp.set(playerId, 'isDead', false);

  // Enable respawn again for next death cycle
  mp.set(playerId, 'canRespawn', true);

  // Optional: Restore some health
  const currentHealth = actor.percentages.health * 100 || 0;
  if (currentHealth < 25) {
    mp.set(playerId, 'percentages', {
      health: 0.25, // Revive with 25% health
      magicka: actor.percentages.magicka,
      stamina: actor.percentages.stamina,
    });
  }

  // Remove from downed tracking
  downedPlayers.delete(playerId);

  // Log the revival
  if (isAutoRevive) {
    console.log(`Player ${playerId} auto-revived after timeout`);
  } else if (healerId) {
    console.log(`Player ${playerId} revived by healer ${healerId}`);
  }
}

/**
 * Find the nearest downed player
 */
function findNearestDownedPlayer(
  fromPos: [number, number, number],
  maxDistance: number,
): { id: number; distance: number } | null {
  let nearest: { id: number; distance: number } | null = null;

  downedPlayers.forEach((_, playerId) => {
    const player = mp.getFormById(playerId) as MpActor;
    if (!player) return;

    const dx = player.pos[0] - fromPos[0];
    const dy = player.pos[1] - fromPos[1];
    const dz = player.pos[2] - fromPos[2];
    const distance = Math.sqrt(dx * dx + dy * dy + dz * dz);

    if (distance <= maxDistance && (!nearest || distance < nearest.distance)) {
      nearest = { id: playerId, distance };
    }
  });

  return nearest;
}

// Alternative approach: Use Papyrus-side death detection
// If you prefer to handle death in Papyrus (.pex):
/*
PAPYRUS EXAMPLE:
================

Event OnInit()
  ; Register for death events
  RegisterForRemoteEvent(GetPlayerRef(), "OnObjectEquipped")
  RegisterForRemoteEvent(GetPlayerRef(), "OnDying")
EndEvent

Event Actor.OnDying(Actor akSelf, Actor akKiller)
  ; Disable respawn
  utility.setIniFloat("fRespawnDelayMax:GamePlay", 999999.0)
  
  ; Call gamemode.js to handle revival
  Debug.SendAnimationEvent(GetPlayerRef(), "blockStart")
EndEvent

Function RevivePlayer()
  ; Called from gamemode when healer revives this player
  GetPlayerRef().SetCriticalStage(0)
  GetPlayerRef().DamageActorValue("Health", -9999.0)
  
  ; Re-enable respawn
  utility.setIniFloat("fRespawnDelayMax:GamePlay", 25.0)
EndFunction
*/
