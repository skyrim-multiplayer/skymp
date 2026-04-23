// ── College of Winterhold ─────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import { signScript } from '../../core/signHelper'
import type { Mp, Store, Bus, LectureSession } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const RANK_THRESHOLDS: Array<{ rank: string; xp: number }> = [
  { rank: 'novice',     xp: 0 },
  { rank: 'apprentice', xp: 100 },
  { rank: 'adept',      xp: 300 },
  { rank: 'expert',     xp: 600 },
  { rank: 'master',     xp: 1000 },
]

const LECTURE_XP_ATTENDEE  = 50
const LECTURE_XP_LECTURER  = 25
const LECTURE_BOOST_MS     = 24 * 60 * 60 * 1000  // 24h
const LECTURE_MAGICKA_MULT = 1.15

// baseId → XP gained from studying that tome
// FormIDs verified against skyrim-esm-references/books.json
const TOME_XP: Record<number, number> = {
  // Novice (15 XP)
  0x0009CD51: 15, // Spell Tome: Flames           edid: SpellTomeFlames
  0x0009CD52: 15, // Spell Tome: Frostbite        edid: SpellTomeFrostbite
  0x0009CD53: 15, // Spell Tome: Sparks           edid: SpellTomeSparks
  // Apprentice (30 XP)
  0x000A26FD: 30, // Spell Tome: Firebolt         edid: SpellTomeFirebolt
  0x000A26FE: 30, // Spell Tome: Ice Spike        edid: SpellTomeIceSpike
  0x000A26FF: 30, // Spell Tome: Lightning Bolt   edid: SpellTomeLightningBolt
  // Adept (50 XP)
  0x000A2706: 50, // Spell Tome: Fireball         edid: SpellTomeFireball
  0x000A2707: 50, // Spell Tome: Ice Storm        edid: SpellTomeIceStorm
  0x000A2708: 50, // Spell Tome: Chain Lightning  edid: SpellTomeChainLightning
  0x0010F7F4: 50, // Spell Tome: Incinerate       edid: SpellTomeIncinerate
  // Expert (75 XP)
  0x000A2709: 75, // Spell Tome: Wall of Flames   edid: SpellTomeWallOfFlames
  0x000A270A: 75, // Spell Tome: Wall of Frost    edid: SpellTomeWallOfFrost
  0x000A270B: 75, // Spell Tome: Wall of Storms   edid: SpellTomeWallOfStorms
  0x0010F7F3: 75, // Spell Tome: Icy Spear        edid: SpellTomeIcySpear
  0x0010F7F5: 75, // Spell Tome: Thunderbolt      edid: SpellTomeThunderbolt
  // Master (100 XP)
  0x000A270C: 100, // Spell Tome: Fire Storm      edid: SpellTomeFireStorm
  0x000A270D: 100, // Spell Tome: Blizzard        edid: SpellTomeBlizzard
  0x000A270E: 100, // Spell Tome: Lightning Storm edid: SpellTomeLightningStorm
}

// ── In-memory lecture sessions ────────────────────────────────────────────────
// lecturerId → { attendees: Set<userId> }
const lectures = new Map<number, LectureSession>()

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function getCollegeRank(xp: number): string {
  let rank = 'novice'
  for (const t of RANK_THRESHOLDS) {
    if (xp >= t.xp) rank = t.rank
  }
  return rank
}

export function getTomeRank(tomeBaseId: number): string | null {
  const xp = TOME_XP[tomeBaseId]
  if (xp === undefined) return null
  if (xp >= 100) return 'master'
  if (xp >= 75)  return 'expert'
  if (xp >= 50)  return 'adept'
  if (xp >= 30)  return 'apprentice'
  return 'novice'
}

export function getStudyXp(mp: Mp, store: Store, playerId: number): number {
  const player = store.get(playerId)
  if (!player) return 0
  return safeGet<number>(mp, player.actorId, 'ff_study_xp', 0)
}

export function getCollegeRankForPlayer(mp: Mp, store: Store, playerId: number): string {
  return getCollegeRank(getStudyXp(mp, store, playerId))
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function studyTome(mp: Mp, store: Store, bus: Bus, playerId: number, tomeBaseId: number): void {
  const player = store.get(playerId)
  if (!player) return
  const xpGain = TOME_XP[tomeBaseId]
  if (xpGain === undefined) return

  if (!player.actorId) return
  const current = getStudyXp(mp, store, playerId)
  const newXp   = current + xpGain
  safeSet(mp, player.actorId, 'ff_study_xp', newXp)
  bus.dispatch({ type: 'collegeXpGained', playerId, xpGain, totalXp: newXp })
}

export function startLecture(mp: Mp, store: Store, bus: Bus, lecturerId: number): boolean {
  if (lectures.has(lecturerId)) return false
  lectures.set(lecturerId, { attendees: new Set() })
  bus.dispatch({ type: 'lectureStarted', lecturerId })
  return true
}

export function joinLecture(mp: Mp, store: Store, bus: Bus, playerId: number, lecturerId: number): boolean {
  const session = lectures.get(lecturerId)
  if (!session) return false
  if (playerId === lecturerId) return false
  session.attendees.add(playerId)
  bus.dispatch({ type: 'lectureJoined', playerId, lecturerId })
  return true
}

export function endLecture(mp: Mp, store: Store, bus: Bus, lecturerId: number, now?: number): boolean {
  const session = lectures.get(lecturerId)
  if (!session) return false

  const boostExpiry = (now ?? Date.now()) + LECTURE_BOOST_MS

  // Award XP + boost to attendees
  for (const attendeeId of session.attendees) {
    const attendee = store.get(attendeeId)
    if (!attendee || !attendee.actorId) continue
    const current = getStudyXp(mp, store, attendeeId)
    safeSet(mp, attendee.actorId, 'ff_study_xp', current + LECTURE_XP_ATTENDEE)
    safeSet(mp, attendee.actorId, 'ff_lecture_boost', boostExpiry)
    bus.dispatch({ type: 'lectureXpGained', playerId: attendeeId, xpGain: LECTURE_XP_ATTENDEE })
  }

  // Award XP only to lecturer
  const lecturer = store.get(lecturerId)
  if (lecturer && lecturer.actorId) {
    const current = getStudyXp(mp, store, lecturerId)
    safeSet(mp, lecturer.actorId, 'ff_study_xp', current + LECTURE_XP_LECTURER)
    bus.dispatch({ type: 'lectureXpGained', playerId: lecturerId, xpGain: LECTURE_XP_LECTURER })
  }

  lectures.delete(lecturerId)
  bus.dispatch({ type: 'lectureEnded', lecturerId, attendeeCount: session.attendees.size })
  return true
}

export function getActiveLecture(lecturerId: number): LectureSession | null {
  return lectures.get(lecturerId) ?? null
}

export function hasLectureBoost(mp: Mp, store: Store, playerId: number, now?: number): boolean {
  const player = store.get(playerId)
  if (!player) return false
  const expiry = safeGet<number>(mp, player.actorId, 'ff_lecture_boost', 0)
  if (!expiry) return false
  return (now ?? Date.now()) < expiry
}

export function getLectureBoostRemainingMs(mp: Mp, store: Store, playerId: number, now?: number): number {
  const player = store.get(playerId)
  if (!player) return 0
  const expiry = safeGet<number>(mp, player.actorId, 'ff_lecture_boost', 0)
  if (!expiry) return 0
  return Math.max(0, expiry - (now ?? Date.now()))
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[college] Initializing')

  mp.makeProperty('ff_study_xp', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: '',
    updateNeighbor: '',
  })

  mp.makeProperty('ff_lecture_boost', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: signScript(`
      (() => {
        const expiry = ctx.value;
        const now    = Date.now();
        if (!expiry || now >= expiry) return { magickaRegenMult: 1.0, boostActive: false };
        return { magickaRegenMult: ${LECTURE_MAGICKA_MULT}, boostActive: true };
      })()
    `),
    updateNeighbor: '',
  })

  console.log('[college] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player || !player.actorId) return
  const xp   = safeGet<number>(mp, player.actorId, 'ff_study_xp', 0)
  const rank = getCollegeRank(xp)
  mp.sendCustomPacket(player.actorId, 'collegeSync', { xp, rank })
}
