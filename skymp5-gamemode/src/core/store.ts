// ── Player Store ──────────────────────────────────────────────────────────────
// In-memory state for all connected players, keyed by SkyMP userId.
// Cleared on disconnect — persistent data lives in mp.set / mp.get.

import type { PlayerState, Store } from '../types'

const players = new Map<number, PlayerState>()

function defaultState(id: number, actorId: number, name: string): PlayerState {
  return {
    id,
    actorId,
    name,
    holdId:           null,
    factions:         [],
    bounty:           {},
    isDown:           false,
    isCaptive:        false,
    downedAt:         null,
    captiveAt:        null,
    properties:       [],
    hungerLevel:      10,
    drunkLevel:       0,
    septims:          0,
    stipendPaidHours: 0,
    minutesOnline:    0,
    isStaff:          false,
    isLeader:         false,
  }
}

function register(id: number, actorId: number, name: string): void {
  players.set(id, defaultState(id, actorId, name))
}

function deregister(id: number): void {
  players.delete(id)
}

function get(id: number): PlayerState | null {
  return players.get(id) ?? null
}

function getAll(): PlayerState[] {
  return Array.from(players.values())
}

function update(id: number, patch: Partial<PlayerState>): void {
  const player = players.get(id)
  if (!player) throw new Error(`store.update: unknown player ${id}`)
  Object.assign(player, patch)
}

export const store: Store = { register, deregister, get, getAll, update }
