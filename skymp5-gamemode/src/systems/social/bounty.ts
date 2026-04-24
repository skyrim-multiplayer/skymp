// ── Bounty ────────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import type { Mp, Store, Bus } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const GUARD_KOID_THRESHOLD = 1000  // Septims; guard gets KOID at or above this

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function getBounty(mp: Mp, store: Store, playerId: number, holdId: string): number {
  const player = store.get(playerId)
  if (!player) return 0
  return player.bounty[holdId] ?? 0
}

export function getAllBounties(mp: Mp, store: Store, playerId: number): Record<string, number> {
  const player = store.get(playerId)
  if (!player) return {}
  return Object.assign({}, player.bounty)
}

export function isGuardKoid(mp: Mp, store: Store, playerId: number, holdId: string): boolean {
  return getBounty(mp, store, playerId, holdId) >= GUARD_KOID_THRESHOLD
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function addBounty(mp: Mp, store: Store, bus: Bus, playerId: number, holdId: string, amount: number): void {
  const player = store.get(playerId)
  if (!player) return
  const current = player.bounty[holdId] ?? 0
  const newAmount = current + amount
  const newBounty = Object.assign({}, player.bounty, { [holdId]: newAmount })
  store.update(playerId, { bounty: newBounty })
  _persist(mp, player.actorId, newBounty)
  if (player.actorId) mp.sendCustomPacket(player.actorId, 'bountyChanged', { holdId, amount: newAmount })
  bus.dispatch({ type: 'bountyChanged', playerId, holdId, newAmount, delta: amount })
}

export function clearBounty(mp: Mp, store: Store, bus: Bus, playerId: number, holdId: string): void {
  const player = store.get(playerId)
  if (!player) return
  const newBounty = Object.assign({}, player.bounty, { [holdId]: 0 })
  store.update(playerId, { bounty: newBounty })
  _persist(mp, player.actorId, newBounty)
  if (player.actorId) mp.sendCustomPacket(player.actorId, 'bountyChanged', { holdId, amount: 0 })
  bus.dispatch({ type: 'bountyChanged', playerId, holdId, newAmount: 0, delta: -(player.bounty[holdId] ?? 0) })
}

// ── Internal ──────────────────────────────────────────────────────────────────

function _persist(mp: Mp, actorId: number, bountyMap: Record<string, number>): void {
  const records = Object.entries(bountyMap)
    .filter(([, amount]) => amount > 0)
    .map(([holdId, amount]) => ({ holdId, amount, updatedAt: Date.now() }))
  safeSet(mp, actorId, 'ff_bounty', records)
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[bounty] Initializing')
  console.log('[bounty] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player) return
  const records: Array<{ holdId: string; amount: number }> = safeGet(mp, player.actorId, 'ff_bounty', [])
  const bountyMap: Record<string, number> = {}
  for (const r of records) bountyMap[r.holdId] = r.amount
  store.update(userId, { bounty: bountyMap })
}
