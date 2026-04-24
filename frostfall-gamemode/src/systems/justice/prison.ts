// ── Prison ────────────────────────────────────────────────────────────────────

import * as worldStore from '../../core/worldStore'
import * as courier from '../communication/courier'
import { safeSet } from '../../core/mpUtil'
import type { Mp, Store, Bus, PrisonQueueEntry, Sentence } from '../../types'

// ── State ─────────────────────────────────────────────────────────────────────
let queue: PrisonQueueEntry[] = []

// ── Accessors ─────────────────────────────────────────────────────────────────

export function getQueue(mp: Mp, holdId?: string): PrisonQueueEntry[] {
  if (holdId) return queue.filter(e => e.holdId === holdId)
  return queue.slice()
}

export function isQueued(mp: Mp, playerId: number): boolean {
  return queue.some(e => e.playerId === playerId)
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function queueForSentencing(mp: Mp, store: Store, bus: Bus, playerId: number, holdId: string, arrestingOfficerId: number, notifyId: number): boolean {
  if (isQueued(mp, playerId)) return false

  const entry: PrisonQueueEntry = { playerId, holdId, arrestedBy: arrestingOfficerId, queuedAt: Date.now() }
  queue.push(entry)
  _persist()

  const note = courier.createNotification(
    'prisonRequest', playerId, notifyId, holdId,
    { playerId, arrestedBy: arrestingOfficerId }
  )
  courier.sendNotification(mp, store, note)
  bus.dispatch({ type: 'playerArrested', playerId, holdId, arrestedBy: arrestingOfficerId })
  return true
}

export function sentencePlayer(mp: Mp, store: Store, bus: Bus, playerId: number, jarlId: number, sentence: Sentence): boolean {
  const entry = queue.find(e => e.playerId === playerId)
  if (!entry) return false

  const { holdId } = entry
  queue = queue.filter(e => e.playerId !== playerId)
  _persist()

  const player = store.get(playerId)

  if (sentence.type === 'fine') {
    const fineAmount = Math.min(sentence.fineAmount ?? 0, player ? player.septims : 0)
    if (player && fineAmount > 0) {
      const newSeptims = player.septims - fineAmount
      store.update(playerId, { septims: newSeptims })
      const newBounty = Object.assign({}, player.bounty, { [holdId]: 0 })
      store.update(playerId, { bounty: newBounty })
      safeSet(mp, player.actorId, 'ff_bounty', [])
    }
  } else if (sentence.type === 'release') {
    if (player) {
      const newBounty = Object.assign({}, player.bounty, { [holdId]: 0 })
      store.update(playerId, { bounty: newBounty })
    }
  } else if (sentence.type === 'banish') {
    if (player) {
      const newBounty = Object.assign({}, player.bounty, { [holdId]: 0 })
      store.update(playerId, { bounty: newBounty })
      mp.sendCustomPacket(player.actorId, 'playerBanished', { holdId })
    }
  }

  bus.dispatch({ type: 'playerSentenced', playerId, jarlId, holdId, sentence })
  return true
}

// ── Internal ──────────────────────────────────────────────────────────────────

function _persist(): void {
  worldStore.set('ff_prison_queue', queue)
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[prison] Initializing')

  const saved = worldStore.get<PrisonQueueEntry[]>('ff_prison_queue')
  if (Array.isArray(saved)) queue = saved

  console.log('[prison] Started')
}
