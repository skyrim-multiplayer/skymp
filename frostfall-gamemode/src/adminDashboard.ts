import { safeGet, safeSet } from './core/mpUtil'
import type { Bus, FactionDocument, FactionMembership, Inventory, Mp, PlayerState, Store } from './types'
import * as bounty from './systems/social/bounty'
import * as factions from './systems/social/factions'

type AdminOk<T> = { ok: true; data: T }
type AdminErr = { ok: false; error: string }
type AdminResult<T> = AdminOk<T> | AdminErr

interface AdminPlayerSnapshot extends PlayerState {
  guid: string | null
  ip: string | null
  connected: boolean
  pos: [number, number, number] | null
  cellOrWorld: number | null
  memberships: FactionMembership[]
  actorProperties: Record<string, unknown>
}

interface AdminSnapshot {
  players: AdminPlayerSnapshot[]
  editableFields: Array<{ key: string; type: 'string' | 'number' | 'boolean'; min?: number; max?: number }>
  knownActorProperties: string[]
}

const KNOWN_ACTOR_PROPERTIES = [
  'ff_hunger',
  'ff_drunk',
  'ff_stipendHours',
  'ff_memberships',
  'ff_bounty',
  'ff_skill_xp',
  'ff_study_xp',
  'ff_study_boosts',
  'ff_lecture_boost',
  'ff_courier',
  'inventory',
]

const EDITABLE_FIELDS: AdminSnapshot['editableFields'] = [
  { key: 'name', type: 'string' },
  { key: 'holdId', type: 'string' },
  { key: 'hungerLevel', type: 'number', min: 0, max: 10 },
  { key: 'drunkLevel', type: 'number', min: 0, max: 10 },
  { key: 'septims', type: 'number', min: 0 },
  { key: 'stipendPaidHours', type: 'number', min: 0, max: 24 },
  { key: 'minutesOnline', type: 'number', min: 0 },
  { key: 'isStaff', type: 'boolean' },
  { key: 'isLeader', type: 'boolean' },
  { key: 'isDown', type: 'boolean' },
  { key: 'isCaptive', type: 'boolean' },
]

const GOLD_BASE_ID = 0x0000000F

declare global {
  // eslint-disable-next-line no-var
  var frostfallAdminDashboard: AdminDashboardApi | undefined
}

interface AdminDashboardApi {
  getSnapshot(): AdminResult<AdminSnapshot>
  patchPlayer(input: unknown): AdminResult<AdminPlayerSnapshot>
  setFaction(input: unknown): AdminResult<AdminPlayerSnapshot>
  removeFaction(input: unknown): AdminResult<AdminPlayerSnapshot>
  setBounty(input: unknown): AdminResult<AdminPlayerSnapshot>
  setActorProperty(input: unknown): AdminResult<AdminPlayerSnapshot>
  setFactionDocument(input: unknown): AdminResult<FactionDocument>
}

export function init(mp: Mp, store: Store, bus: Bus): void {
  globalThis.frostfallAdminDashboard = {
    getSnapshot: () => guarded(() => ({
      players: store.getAll().map(player => snapshotPlayer(mp, store, player)),
      editableFields: EDITABLE_FIELDS,
      knownActorProperties: KNOWN_ACTOR_PROPERTIES,
    })),

    patchPlayer: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const playerId = asPlayerId(data.playerId)
      const player = requirePlayer(store, playerId)
      const patchInput = asRecord(data.patch)
      const patch: Partial<PlayerState> = {}

      for (const field of EDITABLE_FIELDS) {
        if (!(field.key in patchInput)) continue
        const value = patchInput[field.key]
        if (field.type === 'boolean') {
          ;(patch as Record<string, unknown>)[field.key] = Boolean(value)
        } else if (field.type === 'number') {
          ;(patch as Record<string, unknown>)[field.key] = clampNumber(value, field.min, field.max)
        } else {
          ;(patch as Record<string, unknown>)[field.key] =
            field.key === 'holdId' ? normalizeNullableString(value) : normalizeString(value)
        }
      }

      if ('isDown' in patch) patch.downedAt = patch.isDown ? (player.downedAt ?? Date.now()) : null
      if ('isCaptive' in patch) patch.captiveAt = patch.isCaptive ? (player.captiveAt ?? Date.now()) : null

      if (Object.keys(patch).length > 0) store.update(playerId, patch)
      syncPatchToActor(mp, store.get(playerId)!)
      return snapshotPlayer(mp, store, store.get(playerId)!)
    }),

    setFaction: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const playerId = asPlayerId(data.playerId)
      const factionId = normalizeId(data.factionId, 'factionId')
      const rank = clampNumber(data.rank ?? 0, -100, 100)
      if (!factions.joinFaction(mp, store, bus, playerId, factionId, rank)) {
        throw new Error(`Player ${playerId} is not connected`)
      }
      return snapshotPlayer(mp, store, requirePlayer(store, playerId))
    }),

    removeFaction: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const playerId = asPlayerId(data.playerId)
      const factionId = normalizeId(data.factionId, 'factionId')
      if (!factions.leaveFaction(mp, store, bus, playerId, factionId)) {
        throw new Error(`Player ${playerId} is not connected`)
      }
      return snapshotPlayer(mp, store, requirePlayer(store, playerId))
    }),

    setBounty: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const playerId = asPlayerId(data.playerId)
      const holdId = normalizeId(data.holdId, 'holdId')
      const amount = clampNumber(data.amount, 0)
      const current = bounty.getBounty(mp, store, playerId, holdId)
      if (amount > current) bounty.addBounty(mp, store, bus, playerId, holdId, amount - current)
      if (amount < current) bounty.addBounty(mp, store, bus, playerId, holdId, amount - current)
      if (amount === 0) bounty.clearBounty(mp, store, bus, playerId, holdId)
      return snapshotPlayer(mp, store, requirePlayer(store, playerId))
    }),

    setActorProperty: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const playerId = asPlayerId(data.playerId)
      const key = normalizePropertyKey(data.key)
      const player = requirePlayer(store, playerId)
      if (!safeSet(mp, player.actorId, key, data.value)) {
        throw new Error(`Could not set ${key}; actor is not ready or the form is unavailable`)
      }
      syncKnownPropertyToStore(mp, store, playerId, key, data.value)
      return snapshotPlayer(mp, store, requirePlayer(store, playerId))
    }),

    setFactionDocument: (input: unknown) => guarded(() => {
      const data = asRecord(input)
      const doc: FactionDocument = {
        factionId: normalizeId(data.factionId, 'factionId'),
        benefits: String(data.benefits ?? ''),
        burdens: String(data.burdens ?? ''),
        bylaws: String(data.bylaws ?? ''),
      }
      factions.setFactionDocument(mp, doc)
      return factions.getFactionDocument(mp, doc.factionId) ?? doc
    }),
  }

  console.log('[admin] Dashboard API registered')
}

function guarded<T>(fn: () => T): AdminResult<T> {
  try {
    return { ok: true, data: fn() }
  } catch (err: any) {
    return { ok: false, error: String(err?.message ?? err) }
  }
}

function snapshotPlayer(mp: Mp, store: Store, player: PlayerState): AdminPlayerSnapshot {
  const actorProperties: Record<string, unknown> = {}
  for (const key of KNOWN_ACTOR_PROPERTIES) {
    actorProperties[key] = safeGet<unknown>(mp, player.actorId, key, null)
  }

  return Object.assign({}, player, {
    guid: safeCall(() => mp.getUserGuid(player.id), null),
    ip: safeCall(() => mp.getUserIp(player.id), null),
    connected: true,
    pos: safeCall(() => mp.getActorPos(player.actorId) as [number, number, number], null),
    cellOrWorld: safeCall(() => mp.getActorCellOrWorld(player.actorId), null),
    memberships: factions.getPlayerMemberships(mp, store, player.id),
    actorProperties,
  })
}

function safeCall<T>(fn: () => T, fallback: T): T {
  try {
    const value = fn()
    return value === undefined ? fallback : value
  } catch {
    return fallback
  }
}

function asRecord(value: unknown): Record<string, any> {
  if (!value || typeof value !== 'object' || Array.isArray(value)) {
    throw new Error('Expected an object payload')
  }
  return value as Record<string, any>
}

function asPlayerId(value: unknown): number {
  const id = Number(value)
  if (!Number.isInteger(id) || id < 0) throw new Error('Invalid playerId')
  return id
}

function requirePlayer(store: Store, playerId: number): PlayerState {
  const player = store.get(playerId)
  if (!player) throw new Error(`Player ${playerId} is not connected`)
  return player
}

function clampNumber(value: unknown, min?: number, max?: number): number {
  const n = Number(value)
  if (!Number.isFinite(n)) throw new Error(`Invalid number: ${value}`)
  const whole = Math.trunc(n)
  return Math.max(min ?? whole, Math.min(max ?? whole, whole))
}

function normalizeNullableString(value: unknown): string | null {
  const text = String(value ?? '').trim()
  return text.length > 0 ? text.slice(0, 96) : null
}

function normalizeString(value: unknown): string {
  const text = String(value ?? '').trim()
  return text.length > 0 ? text.slice(0, 96) : 'Unknown'
}

function normalizeId(value: unknown, label: string): string {
  const text = String(value ?? '').trim().toLowerCase()
  if (!/^[a-z0-9_.:-]{1,64}$/.test(text)) {
    throw new Error(`${label} must be 1-64 chars: a-z, 0-9, _, ., :, or -`)
  }
  return text
}

function normalizePropertyKey(value: unknown): string {
  const text = String(value ?? '').trim()
  if (!/^[A-Za-z0-9_.:-]{1,64}$/.test(text)) {
    throw new Error('Property key must be 1-64 chars: letters, numbers, _, ., :, or -')
  }
  return text
}

function syncPatchToActor(mp: Mp, player: PlayerState): void {
  safeSet(mp, player.actorId, 'ff_hunger', player.hungerLevel)
  safeSet(mp, player.actorId, 'ff_drunk', player.drunkLevel)
  safeSet(mp, player.actorId, 'ff_stipendHours', player.stipendPaidHours)

  const inv = safeGet<Inventory | null>(mp, player.actorId, 'inventory', null)
  safeSet(mp, player.actorId, 'inventory', setGoldInInventory(inv, player.septims))
}

function syncKnownPropertyToStore(mp: Mp, store: Store, playerId: number, key: string, value: unknown): void {
  const patch: Partial<PlayerState> = {}
  if (key === 'name') patch.name = String(value ?? '').slice(0, 96)
  if (key === 'ff_hunger') patch.hungerLevel = clampNumber(value, 0, 10)
  if (key === 'ff_drunk') patch.drunkLevel = clampNumber(value, 0, 10)
  if (key === 'ff_stipendHours') patch.stipendPaidHours = clampNumber(value, 0, 24)
  if (key === 'ff_memberships' && Array.isArray(value)) {
    patch.factions = value.map((m: any) => String(m?.factionId ?? '')).filter(Boolean)
  }
  if (key === 'ff_bounty' && Array.isArray(value)) {
    const bountyMap: Record<string, number> = {}
    for (const item of value) {
      const holdId = String(item?.holdId ?? '').trim().toLowerCase()
      const amount = Number(item?.amount ?? 0)
      if (holdId && Number.isFinite(amount)) bountyMap[holdId] = Math.max(0, Math.trunc(amount))
    }
    patch.bounty = bountyMap
  }
  if (key === 'inventory') {
    patch.septims = getGoldFromInventory(value as Inventory | null)
  }
  if (Object.keys(patch).length > 0) store.update(playerId, patch)

  const player = store.get(playerId)
  if (player) syncPatchToActor(mp, player)
}

function getGoldFromInventory(inv: Inventory | null): number {
  if (!inv || !Array.isArray(inv.entries)) return 0
  const entry = inv.entries.find(e => e.baseId === GOLD_BASE_ID)
  return entry ? Math.max(0, Math.trunc(Number(entry.count) || 0)) : 0
}

function setGoldInInventory(inv: Inventory | null, amount: number): Inventory {
  const entries = (inv && Array.isArray(inv.entries)) ? inv.entries.filter(e => e.baseId !== GOLD_BASE_ID) : []
  if (amount > 0) entries.push({ baseId: GOLD_BASE_ID, count: amount })
  return { entries }
}
