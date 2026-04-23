// ── Training ──────────────────────────────────────────────────────────────────

import * as skills from './skills'
import type { Mp, Store, Bus, TrainingSession } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const TRAINING_BOOST_MULTIPLIER = 2.0
const TRAINING_BOOST_ONLINE_MS  = 24 * 60 * 60 * 1000  // 24h of online time
const TRAINING_LOCATION_RADIUS  = 500  // Skyrim units

// ── In-memory sessions ────────────────────────────────────────────────────────
// trainerId → { skillId, attendees: Set<userId> }
const sessions = new Map<number, TrainingSession>()

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function getActiveTraining(trainerId: number): TrainingSession | null {
  return sessions.get(trainerId) ?? null
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function startTraining(mp: Mp, store: Store, bus: Bus, trainerId: number, skillId: string): boolean {
  if (sessions.has(trainerId)) return false
  sessions.set(trainerId, { skillId, attendees: new Set() })
  bus.dispatch({ type: 'trainingStarted', trainerId, skillId })
  return true
}

export function joinTraining(mp: Mp, store: Store, bus: Bus, playerId: number, trainerId: number): boolean {
  const session = sessions.get(trainerId)
  if (!session) return false
  if (playerId === trainerId) return false

  const player  = store.get(playerId)
  const trainer = store.get(trainerId)
  if (!player || !trainer) return false

  // Location check — only if positional data is available
  try {
    const playerPos  = mp.getActorPos(player.actorId)
    const trainerPos = mp.getActorPos(trainer.actorId)
    if (playerPos && trainerPos) {
      const dx = playerPos[0] - trainerPos[0]
      const dy = playerPos[1] - trainerPos[1]
      const dz = playerPos[2] - trainerPos[2]
      if (Math.sqrt(dx*dx + dy*dy + dz*dz) > TRAINING_LOCATION_RADIUS) return false
    }
  } catch {}

  session.attendees.add(playerId)
  bus.dispatch({ type: 'trainingJoined', playerId, trainerId })
  return true
}

export function endTraining(mp: Mp, store: Store, bus: Bus, trainerId: number): boolean {
  const session = sessions.get(trainerId)
  if (!session) return false

  for (const attendeeId of session.attendees) {
    skills.grantStudyBoost(mp, attendeeId, session.skillId, TRAINING_BOOST_MULTIPLIER, TRAINING_BOOST_ONLINE_MS)
    const attendee = store.get(attendeeId)
    if (attendee) mp.sendCustomPacket(attendee.actorId, 'trainingBoostGranted', { skillId: session.skillId })
  }

  sessions.delete(trainerId)
  bus.dispatch({ type: 'trainingEnded', trainerId, skillId: session.skillId, attendeeCount: session.attendees.size })
  return true
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[training] Initializing')
  // Sessions are in-memory only — intentionally do not persist across restarts
  console.log('[training] Started')
}
