// ── Hunger ────────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import { signScript } from '../../core/signHelper'
import type { Mp, Store, Bus } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const HUNGER_MIN         = 0
const HUNGER_MAX         = 10
const DRAIN_INTERVAL_MIN = 30   // drain 1 level every 30 minutes of playtime
const TICK_INTERVAL_MS   = 60 * 1000

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function calcNewHunger(current: number, delta: number): number {
  return Math.max(HUNGER_MIN, Math.min(HUNGER_MAX, current + delta))
}

export function shouldDrainHunger(minutesOnline: number): boolean {
  return minutesOnline > 0 && minutesOnline % DRAIN_INTERVAL_MIN === 0
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function feedPlayer(mp: Mp, store: Store, bus: Bus, playerId: number, levels: number): number {
  const player = store.get(playerId)
  if (!player || !player.actorId) return -1
  const newLevel = calcNewHunger(player.hungerLevel, levels)
  store.update(playerId, { hungerLevel: newLevel })
  safeSet(mp, player.actorId, 'ff_hunger', newLevel)
  bus.dispatch({ type: 'hungerTick', playerId, hungerLevel: newLevel })
  return newLevel
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[hunger] Initializing')

  mp.makeProperty('ff_hunger', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: signScript(`
      (() => {
        const h = ctx.value;
        if (h === null || h === undefined) return;
        if (h <= 2) return { healthRegenMult: 0.7 };
        if (h >= 10) return { staminaRegenMult: 1.4 };
        return {};
      })()
    `),
    updateNeighbor: '',
  })

  const scheduleTick = () => {
    setTimeout(() => {
      try {
        for (const player of store.getAll()) {
          store.update(player.id, { minutesOnline: player.minutesOnline + 1 })
          const updated = store.get(player.id)!
          if (shouldDrainHunger(updated.minutesOnline) && updated.actorId) {
            const newLevel = calcNewHunger(updated.hungerLevel, -1)
            store.update(player.id, { hungerLevel: newLevel })
            safeSet(mp, updated.actorId, 'ff_hunger', newLevel)
            bus.dispatch({ type: 'hungerTick', playerId: player.id, hungerLevel: newLevel })
          }
        }
      } catch (err: any) {
        console.error(`[hunger] Tick error: ${err.message}`)
      }
      scheduleTick()
    }, TICK_INTERVAL_MS)
  }

  scheduleTick()
  console.log('[hunger] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player) return
  const level = safeGet(mp, player.actorId, 'ff_hunger', HUNGER_MAX)
  store.update(userId, { hungerLevel: level })
}
