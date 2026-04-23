// ── Drunk Bar ─────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import { signScript } from '../../core/signHelper'
import type { Mp, Store, Bus } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const DRUNK_MIN          = 0
const DRUNK_MAX          = 10
const SOBER_INTERVAL_MIN = 5    // sober tick every 5 minutes of playtime
const TICK_INTERVAL_MS   = 60 * 1000

// baseId → alcohol strength (1–3)
// FormIDs verified against skyrim-esm-references/potions.json
const ALCOHOL_STRENGTHS: Record<number, number> = {
  0x0003133B: 1, // Alto Wine          edid: FoodWineAlto
  0x000C5349: 1, // Alto Wine (var.)   edid: FoodWineAltoA
  0x0003133C: 1, // Wine               edid: FoodWineBottle02
  0x000C5348: 1, // Wine (var.)        edid: FoodWineBottle02A
  0x00034C5D: 2, // Nord Mead          edid: FoodMead
  0x0002C35A: 2, // Black-Briar Mead   edid: FoodBlackBriarMead
  0x000508CA: 2, // Honningbrew Mead   edid: FoodHonningbrewMead
  0x000F693F: 3, // Black-Briar Reserve edid: FoodBlackBriarMeadPrivateReserve
}

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function calcNewDrunkLevel(current: number, delta: number): number {
  return Math.max(DRUNK_MIN, Math.min(DRUNK_MAX, current + delta))
}

export function shouldSober(minutesOnline: number): boolean {
  return minutesOnline > 0 && minutesOnline % SOBER_INTERVAL_MIN === 0
}

export function getAlcoholStrength(baseId: number): number {
  return ALCOHOL_STRENGTHS[baseId] ?? 0
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function drinkAlcohol(mp: Mp, store: Store, bus: Bus, playerId: number, baseId: number): void {
  const player = store.get(playerId)
  if (!player) return
  const strength = getAlcoholStrength(baseId)
  if (!strength) return
  const newLevel = calcNewDrunkLevel(player.drunkLevel, strength)
  store.update(playerId, { drunkLevel: newLevel })
  safeSet(mp, player.actorId, 'ff_drunk', newLevel)
  bus.dispatch({ type: 'drunkChanged', playerId, drunkLevel: newLevel })
}

export function soberPlayer(mp: Mp, store: Store, bus: Bus, playerId: number): void {
  const player = store.get(playerId)
  if (!player) return
  store.update(playerId, { drunkLevel: 0 })
  safeSet(mp, player.actorId, 'ff_drunk', 0)
  bus.dispatch({ type: 'drunkChanged', playerId, drunkLevel: 0 })
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[drunkBar] Initializing')

  mp.makeProperty('ff_drunk', {
    isVisibleByOwner: true,
    isVisibleByNeighbors: false,
    updateOwner: signScript(`
      (() => {
        const d = ctx.value;
        if (d === null || d === undefined) return;
        if (d >= 8) return { weaponSpeedMult: 0.6 };
        if (d >= 5) return { weaponSpeedMult: 0.8 };
        return {};
      })()
    `),
    updateNeighbor: '',
  })

  const scheduleTick = () => {
    setTimeout(() => {
      try {
        for (const player of store.getAll()) {
          if (shouldSober(player.minutesOnline)) {
            if (player.drunkLevel > 0 && player.actorId) {
              const newLevel = calcNewDrunkLevel(player.drunkLevel, -1)
              store.update(player.id, { drunkLevel: newLevel })
              safeSet(mp, player.actorId, 'ff_drunk', newLevel)
              bus.dispatch({ type: 'drunkChanged', playerId: player.id, drunkLevel: newLevel })
            }
          }
        }
      } catch (err: any) {
        console.error(`[drunkBar] Tick error: ${err.message}`)
      }
      scheduleTick()
    }, TICK_INTERVAL_MS)
  }

  scheduleTick()
  console.log('[drunkBar] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player) return
  const level = safeGet(mp, player.actorId, 'ff_drunk', 0)
  store.update(userId, { drunkLevel: level })
}
