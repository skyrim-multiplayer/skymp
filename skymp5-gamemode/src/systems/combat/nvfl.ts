// ── NVFL ──────────────────────────────────────────────────────────────────────
// No Violence For Life — 24-hour protection window after being downed.
// Pure functions; no mp calls, no side effects.

import type { Store } from '../../types'

const NVFL_WINDOW_MS = 24 * 60 * 60 * 1000  // 24 hours

export function isNvflRestricted(store: Store, playerId: number, now?: number): boolean {
  const player = store.get(playerId)
  if (!player || player.downedAt === null) return false
  const ts = now ?? Date.now()
  return (ts - player.downedAt) < NVFL_WINDOW_MS
}

export function getNvflRemainingMs(store: Store, playerId: number, now?: number): number {
  const player = store.get(playerId)
  if (!player || player.downedAt === null) return 0
  const ts      = now ?? Date.now()
  const elapsed = ts - player.downedAt
  return Math.max(0, NVFL_WINDOW_MS - elapsed)
}

export function clearNvfl(store: Store, playerId: number): void {
  const player = store.get(playerId)
  if (!player) return
  store.update(playerId, { downedAt: null })
}
