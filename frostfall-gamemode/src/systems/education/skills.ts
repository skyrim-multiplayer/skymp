// ── Skills ────────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import * as factions from '../social/factions'
import type { Mp, Store, Bus, StudyBoost } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const SKILL_LEVEL_XP = 10
const DEFAULT_CAP_XP = 250  // ~level 25

export const SKILL_IDS = [
  'destruction', 'restoration', 'alteration', 'conjuration', 'illusion',
  'smithing', 'enchanting', 'alchemy',
] as const

export type SkillId = typeof SKILL_IDS[number]

interface FactionCap {
  factionId: string
  minRank: number
  skills: string[]
  cap: number
}

// Faction cap bonuses
const FACTION_CAPS: FactionCap[] = [
  { factionId: 'collegeOfWinterhold', minRank: 1, skills: ['destruction','restoration','alteration','conjuration','illusion'], cap: 500 },
  { factionId: 'collegeOfWinterhold', minRank: 2, skills: ['destruction','restoration','alteration','conjuration','illusion'], cap: 750 },
  { factionId: 'collegeOfWinterhold', minRank: 3, skills: ['destruction','restoration','alteration','conjuration','illusion'], cap: 1000 },
  { factionId: 'companions',          minRank: 1, skills: ['smithing'], cap: 500 },
  { factionId: 'companions',          minRank: 2, skills: ['smithing'], cap: 750 },
  { factionId: 'companions',          minRank: 3, skills: ['smithing'], cap: 1000 },
  { factionId: 'eastEmpireCompany',   minRank: 1, skills: ['smithing','enchanting','alchemy'], cap: 500 },
  { factionId: 'eastEmpireCompany',   minRank: 2, skills: ['smithing','enchanting','alchemy'], cap: 750 },
  { factionId: 'thievesGuild',        minRank: 1, skills: ['alchemy'], cap: 500 },
  { factionId: 'thievesGuild',        minRank: 2, skills: ['alchemy'], cap: 750 },
  { factionId: 'bardsCollege',        minRank: 1, skills: ['enchanting'], cap: 500 },
  { factionId: 'bardsCollege',        minRank: 2, skills: ['enchanting'], cap: 750 },
]

// ── In-memory session tracking ─────────────────────────────────────────────────
// userId → session start timestamp (wall clock)
const sessionStart = new Map<number, number>()

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function getSkillLevel(xp: number): number {
  return Math.floor(xp / SKILL_LEVEL_XP)
}

export function getSkillXp(mp: Mp, playerId: number, skillId: string): number {
  const actorId = _actorForPlayer(mp, playerId)
  if (!actorId) return 0
  const xpMap = safeGet<Record<string, number>>(mp, actorId, 'ff_skill_xp', {})
  return xpMap[skillId] ?? 0
}

export function getSkillCap(mp: Mp, store: Store, playerId: number, skillId: string): number {
  let cap = DEFAULT_CAP_XP
  for (const rule of FACTION_CAPS) {
    if (!rule.skills.includes(skillId)) continue
    const rank = factions.getPlayerFactionRank(mp, store, playerId, rule.factionId)
    if (rank !== null && rank >= rule.minRank && rule.cap > cap) {
      cap = rule.cap
    }
  }
  return cap
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function addSkillXp(mp: Mp, store: Store, playerId: number, skillId: string, baseXp: number, now?: number): number {
  const player = store.get(playerId)
  if (!player) return 0
  const cap     = getSkillCap(mp, store, playerId, skillId)
  const current = getSkillXp(mp, playerId, skillId)
  if (current >= cap) return 0

  let multiplier = 1
  const boost = getActiveStudyBoost(mp, playerId, skillId, now)
  if (boost) multiplier = boost.multiplier

  const gain   = Math.round(baseXp * multiplier)
  const newXp  = Math.min(current + gain, cap)
  const actual = newXp - current

  const xpMap: Record<string, number> = safeGet<Record<string, number>>(mp, player.actorId, 'ff_skill_xp', {})
  xpMap[skillId] = newXp
  safeSet(mp, player.actorId, 'ff_skill_xp', xpMap)
  return actual
}

export function grantStudyBoost(mp: Mp, playerId: number, skillId: string, multiplier: number, onlineMs: number): void {
  const actorId = _actorForPlayer(mp, playerId)
  if (!actorId) return
  const boosts: StudyBoost[] = safeGet<StudyBoost[]>(mp, actorId, 'ff_study_boosts', [])
  boosts.push({ skillId, multiplier, remainingOnlineMs: onlineMs, sessionStart: Date.now() })
  safeSet(mp, actorId, 'ff_study_boosts', boosts)
}

export function getActiveStudyBoost(mp: Mp, playerId: number, skillId: string, now?: number): StudyBoost | null {
  _consumeBoostTime(mp, playerId, now)
  const actorId = _actorForPlayer(mp, playerId)
  if (!actorId) return null
  const boosts: StudyBoost[] = safeGet<StudyBoost[]>(mp, actorId, 'ff_study_boosts', [])
  return boosts.find(b => b.skillId === skillId && b.remainingOnlineMs > 0) ?? null
}

export function getStudyBoosts(mp: Mp, playerId: number): StudyBoost[] {
  const actorId = _actorForPlayer(mp, playerId)
  if (!actorId) return []
  return safeGet<StudyBoost[]>(mp, actorId, 'ff_study_boosts', [])
}

// ── Internal ──────────────────────────────────────────────────────────────────

function _consumeBoostTime(mp: Mp, playerId: number, now?: number): void {
  const actorId = _actorForPlayer(mp, playerId)
  if (!actorId) return
  const boosts: StudyBoost[] = safeGet<StudyBoost[]>(mp, actorId, 'ff_study_boosts', [])
  const start = sessionStart.get(playerId)
  if (!start) return
  const elapsed = (now ?? Date.now()) - start
  const updated = boosts
    .map(b => Object.assign({}, b, { remainingOnlineMs: Math.max(0, b.remainingOnlineMs - elapsed) }))
    .filter(b => b.remainingOnlineMs > 0)
  sessionStart.set(playerId, now ?? Date.now())
  safeSet(mp, actorId, 'ff_study_boosts', updated)
}

export function onSkillPlayerDisconnect(mp: Mp, playerId: number, now?: number): void {
  _consumeBoostTime(mp, playerId, now)
  sessionStart.delete(playerId)
}

function _actorForPlayer(mp: Mp, playerId: number): number {
  try { return mp.getUserActor(playerId) } catch { return 0 }
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[skills] Initializing')

  mp.makeProperty('ff_skill_xp', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: '',
    updateNeighbor: '',
  })

  mp.makeProperty('ff_study_boosts', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: '',
    updateNeighbor: '',
  })

  console.log('[skills] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  sessionStart.set(userId, Date.now())
  const player = store.get(userId)
  if (!player || !player.actorId) return
  const xpMap = safeGet<Record<string, number>>(mp, player.actorId, 'ff_skill_xp', {})
  mp.sendCustomPacket(player.actorId, 'skillsSync', { xpMap })
}
