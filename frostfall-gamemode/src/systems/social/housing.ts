// ── Housing ───────────────────────────────────────────────────────────────────

import * as worldStore from '../../core/worldStore'
import * as courier from '../communication/courier'
import type { Mp, Store, Bus, PropertyDef, PropertyState, PropertyRecord } from '../../types'

// ── Property Registry ─────────────────────────────────────────────────────────
// 16 properties across 9 holds. propertyId is the stable key used everywhere.

const PROPERTY_REGISTRY: PropertyDef[] = [
  // Whiterun
  { id: 'wrun_breezehome',      name: 'Breezehome',            holdId: 'whiterun',   type: 'home' },
  { id: 'wrun_breezeannex',     name: 'Breezehome Annex',      holdId: 'whiterun',   type: 'business' },
  // Eastmarch
  { id: 'east_hjerim',          name: 'Hjerim',                holdId: 'eastmarch',  type: 'home' },
  { id: 'east_windhelm_shop',   name: 'Windhelm Market Stall', holdId: 'eastmarch',  type: 'business' },
  // Rift
  { id: 'rift_honeyside',       name: 'Honeyside',             holdId: 'rift',       type: 'home' },
  { id: 'rift_riften_shop',     name: 'Riften Stall',          holdId: 'rift',       type: 'business' },
  // Reach
  { id: 'reach_vlindrel',       name: 'Vlindrel Hall',         holdId: 'reach',      type: 'home' },
  { id: 'reach_markarth_shop',  name: 'Markarth Stall',        holdId: 'reach',      type: 'business' },
  // Haafingar
  { id: 'haaf_proudspire',      name: 'Proudspire Manor',      holdId: 'haafingar',  type: 'home' },
  { id: 'haaf_solitude_shop',   name: 'Solitude Market',       holdId: 'haafingar',  type: 'business' },
  // Pale
  { id: 'pale_dawnstar_home',   name: 'Dawnstar Cottage',      holdId: 'pale',       type: 'home' },
  { id: 'pale_dawnstar_shop',   name: 'Dawnstar Stall',        holdId: 'pale',       type: 'business' },
  // Falkreath
  { id: 'falk_lakeview',        name: 'Lakeview Manor',        holdId: 'falkreath',  type: 'home' },
  { id: 'falk_falkreath_shop',  name: 'Falkreath Stall',       holdId: 'falkreath',  type: 'business' },
  // Hjaalmarch
  { id: 'hjaal_windstad',       name: 'Windstad Manor',        holdId: 'hjaalmarch', type: 'home' },
  // Winterhold
  { id: 'wint_college_quarters',name: 'College Quarters',      holdId: 'winterhold', type: 'home' },
]

// ── Runtime state ─────────────────────────────────────────────────────────────
const properties = new Map<string, PropertyState>()

function _loadRegistry(): void {
  for (const def of PROPERTY_REGISTRY) {
    if (!properties.has(def.id)) {
      properties.set(def.id, { ownerId: null, pendingOwnerId: null })
    }
  }
}

// ── Pure lookups ──────────────────────────────────────────────────────────────

export function getProperty(id: string): PropertyRecord | null {
  const def   = PROPERTY_REGISTRY.find(p => p.id === id)
  const state = properties.get(id)
  if (!def || !state) return null
  return Object.assign({}, def, state)
}

export function getPropertiesByHold(holdId: string): PropertyRecord[] {
  return PROPERTY_REGISTRY
    .filter(p => p.holdId === holdId)
    .map(p => getProperty(p.id)!)
}

export function getOwnedProperties(playerId: number): PropertyRecord[] {
  return PROPERTY_REGISTRY
    .map(p => getProperty(p.id)!)
    .filter(p => p && p.ownerId === playerId)
}

export function isAvailable(propertyId: string): boolean {
  const state = properties.get(propertyId)
  if (!state) return false
  return state.ownerId === null && state.pendingOwnerId === null
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function requestProperty(mp: Mp, store: Store, bus: Bus, playerId: number, propertyId: string, stewardId: number): boolean {
  if (!isAvailable(propertyId)) return false
  properties.get(propertyId)!.pendingOwnerId = playerId
  _persist()
  const player = store.get(playerId)
  const note = courier.createNotification(
    'propertyRequest', playerId, stewardId, null,
    { propertyId, requesterName: player ? player.name : String(playerId) }
  )
  courier.sendNotification(mp, store, note)
  bus.dispatch({ type: 'propertyRequested', playerId, propertyId })
  return true
}

export function approveProperty(mp: Mp, store: Store, bus: Bus, propertyId: string, approverId: number): boolean {
  const state = properties.get(propertyId)
  if (!state || state.pendingOwnerId === null) return false
  const newOwnerId      = state.pendingOwnerId
  state.ownerId         = newOwnerId
  state.pendingOwnerId  = null
  _persist()

  const player = store.get(newOwnerId)
  if (player) {
    const owned = store.get(newOwnerId)!.properties.concat([propertyId])
    store.update(newOwnerId, { properties: owned })
    mp.sendCustomPacket(player.actorId, 'propertyApproved', { propertyId })
  }
  bus.dispatch({ type: 'propertyApproved', propertyId, newOwnerId, approvedBy: approverId })
  return true
}

export function denyProperty(mp: Mp, propertyId: string): boolean {
  const state = properties.get(propertyId)
  if (!state) return false
  state.pendingOwnerId = null
  _persist()
  return true
}

export function revokeProperty(mp: Mp, store: Store, propertyId: string): boolean {
  const state = properties.get(propertyId)
  if (!state) return false
  const prevOwner       = state.ownerId
  state.ownerId         = null
  state.pendingOwnerId  = null
  _persist()
  if (prevOwner !== null) {
    const player = store.get(prevOwner)
    if (player) {
      const owned = player.properties.filter(id => id !== propertyId)
      store.update(prevOwner, { properties: owned })
    }
  }
  return true
}

// ── Internal ──────────────────────────────────────────────────────────────────

function _persist(): void {
  const data: Array<{ id: string; ownerId: number | null; pendingOwnerId: number | null }> = []
  for (const [id, state] of properties) {
    data.push({ id, ownerId: state.ownerId, pendingOwnerId: state.pendingOwnerId })
  }
  worldStore.set('ff_properties', data)
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[housing] Initializing')
  _loadRegistry()

  const saved = worldStore.get<Array<{ id: string; ownerId: number | null; pendingOwnerId: number | null }>>('ff_properties')
  if (Array.isArray(saved)) {
    for (const entry of saved) {
      if (properties.has(entry.id)) {
        const state = properties.get(entry.id)!
        state.ownerId        = entry.ownerId
        state.pendingOwnerId = entry.pendingOwnerId
      }
    }
  }

  console.log('[housing] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player || !player.actorId) return
  const owned = getOwnedProperties(userId).map(p => p.id)
  store.update(userId, { properties: owned })
  // Send property list for the player's hold (empty list if no hold assigned yet
  // so the client always gets a sync event and knows state is current).
  const list = player.holdId ? getPropertiesByHold(player.holdId) : []
  // 3-arg sendCustomPacket is an undeclared native extension — guard so a missing
  // implementation doesn't abort the rest of the onConnect chain.
  try { mp.sendCustomPacket(player.actorId, 'propertyList', { properties: list }) } catch { /* noop */ }
}
