// ── Captivity ─────────────────────────────────────────────────────────────────

import type { Mp, Store, Bus } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const MAX_CAPTIVITY_MS  = 24 * 60 * 60 * 1000  // 24 hours
const CHECK_INTERVAL_MS = 60 * 1000

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function isCaptive(store: Store, playerId: number): boolean {
  const player = store.get(playerId)
  return player ? player.isCaptive : false
}

export function getCaptivityRemainingMs(store: Store, playerId: number, now?: number): number {
  const player = store.get(playerId)
  if (!player || !player.isCaptive || player.captiveAt === null) return 0
  const ts = now ?? Date.now()
  return Math.max(0, MAX_CAPTIVITY_MS - (ts - player.captiveAt))
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function capturePlayer(mp: Mp, store: Store, bus: Bus, captiveId: number, captorId: number): void {
  const captive = store.get(captiveId)
  const captor  = store.get(captorId)
  if (!captive) return

  const now = Date.now()
  store.update(captiveId, { isCaptive: true, captiveAt: now })

  mp.sendCustomPacket(captive.actorId, 'playerCaptured', { remainingMs: MAX_CAPTIVITY_MS })
  if (captor) mp.sendCustomPacket(captor.actorId, 'playerCaptured', { captiveId })

  bus.dispatch({ type: 'playerCaptured', captiveId, captorId })
}

export function releasePlayer(mp: Mp, store: Store, bus: Bus, captiveId: number): void {
  const captive = store.get(captiveId)
  if (!captive) return

  store.update(captiveId, { isCaptive: false, captiveAt: null })
  mp.sendCustomPacket(captive.actorId, 'playerReleased', {})
  bus.dispatch({ type: 'playerReleased', captiveId })
}

export function checkExpiredCaptivity(mp: Mp, store: Store, bus: Bus, now?: number): number[] {
  const ts       = now ?? Date.now()
  const released: number[] = []
  for (const player of store.getAll()) {
    if (player.isCaptive && player.captiveAt !== null) {
      if ((ts - player.captiveAt) >= MAX_CAPTIVITY_MS) {
        releasePlayer(mp, store, bus, player.id)
        released.push(player.id)
      }
    }
  }
  return released
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[captivity] Initializing')

  const scheduleTick = () => {
    setTimeout(() => {
      try { checkExpiredCaptivity(mp, store, bus) } catch (err: any) {
        console.error(`[captivity] Tick error: ${err.message}`)
      }
      scheduleTick()
    }, CHECK_INTERVAL_MS)
  }

  scheduleTick()
  console.log('[captivity] Started')
}
