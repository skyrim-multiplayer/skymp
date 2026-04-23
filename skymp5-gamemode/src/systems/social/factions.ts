// ── Factions ──────────────────────────────────────────────────────────────────

import * as worldStore from '../../core/worldStore'
import { safeGet, safeSet } from '../../core/mpUtil'
import type { Mp, Store, Bus, FactionMembership, FactionDocument } from '../../types'

// ── Actions ───────────────────────────────────────────────────────────────────

export function getFactionDocument(mp: Mp, factionId: string): FactionDocument | null {
  const docs = worldStore.get<Record<string, FactionDocument>>('ff_faction_docs') ?? {}
  return docs[factionId] ?? null
}

export function setFactionDocument(mp: Mp, doc: FactionDocument): void {
  const docs = worldStore.get<Record<string, FactionDocument>>('ff_faction_docs') ?? {}
  docs[doc.factionId] = Object.assign({}, doc, { updatedAt: Date.now() })
  worldStore.set('ff_faction_docs', docs)
}

export function joinFaction(mp: Mp, store: Store, bus: Bus, playerId: number, factionId: string, rank?: number): boolean {
  const player = store.get(playerId)
  if (!player) return false

  const joinRank     = rank ?? 0
  const memberships  = _getMemberships(mp, player.actorId)
  const existingIdx  = memberships.findIndex(m => m.factionId === factionId)

  if (existingIdx >= 0) {
    memberships[existingIdx].rank = joinRank
  } else {
    memberships.push({ factionId, rank: joinRank, joinedAt: Date.now() })
  }

  _saveMemberships(mp, player.actorId, memberships)

  const factionIds = memberships.map(m => m.factionId)
  store.update(playerId, { factions: factionIds })

  bus.dispatch({ type: 'factionJoined', playerId, factionId, rank: joinRank })
  return true
}

export function leaveFaction(mp: Mp, store: Store, bus: Bus, playerId: number, factionId: string): boolean {
  const player = store.get(playerId)
  if (!player) return false

  const memberships = _getMemberships(mp, player.actorId)
  const filtered    = memberships.filter(m => m.factionId !== factionId)
  _saveMemberships(mp, player.actorId, filtered)

  const factionIds = filtered.map(m => m.factionId)
  store.update(playerId, { factions: factionIds })

  bus.dispatch({ type: 'factionLeft', playerId, factionId })
  return true
}

export function isFactionMember(mp: Mp, store: Store, playerId: number, factionId: string): boolean {
  const player = store.get(playerId)
  if (!player) return false
  return player.factions.includes(factionId)
}

export function getPlayerFactionRank(mp: Mp, store: Store, playerId: number, factionId: string): number | null {
  const player = store.get(playerId)
  if (!player) return null
  const memberships = _getMemberships(mp, player.actorId)
  const m = memberships.find(m => m.factionId === factionId)
  return m ? m.rank : null
}

export function getPlayerMemberships(mp: Mp, store: Store, playerId: number): FactionMembership[] {
  const player = store.get(playerId)
  if (!player) return []
  return _getMemberships(mp, player.actorId)
}

// ── Internal ──────────────────────────────────────────────────────────────────

function _getMemberships(mp: Mp, actorId: number): FactionMembership[] {
  return safeGet<FactionMembership[]>(mp, actorId, 'ff_memberships', [])
}

function _saveMemberships(mp: Mp, actorId: number, memberships: FactionMembership[]): void {
  safeSet(mp, actorId, 'ff_memberships', memberships)
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[factions] Initializing')
  console.log('[factions] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player || !player.actorId) return
  const memberships = _getMemberships(mp, player.actorId)
  const factionIds  = memberships.map(m => m.factionId)
  store.update(userId, { factions: factionIds })
  // 3-arg sendCustomPacket is an undeclared native extension — guard so a missing
  // implementation doesn't abort the rest of the onConnect chain.
  try { mp.sendCustomPacket(player.actorId, 'factionsSync', { memberships }) } catch { /* noop */ }
}
