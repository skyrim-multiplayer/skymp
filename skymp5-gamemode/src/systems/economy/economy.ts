// ── Economy ───────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import type { Mp, Store, Bus, Inventory } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const STIPEND_RATE         = 50   // Septims per hour
const STIPEND_CAP_HOURS    = 24
const STIPEND_INTERVAL_MIN = 60  // pay every 60 minutes of playtime
const TICK_INTERVAL_MS     = 60 * 1000

// ── Pure helpers ──────────────────────────────────────────────────────────────

export function isStipendEligible(stipendPaidHours: number): boolean {
  return stipendPaidHours < STIPEND_CAP_HOURS
}

export function shouldPayStipend(minutesOnline: number, stipendPaidHours: number): boolean {
  if (!isStipendEligible(stipendPaidHours)) return false
  return minutesOnline > 0 && minutesOnline % STIPEND_INTERVAL_MIN === 0
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function transferGold(mp: Mp, store: Store, fromId: number, toId: number, amount: number): boolean {
  if (!amount || amount <= 0) return false
  const from = store.get(fromId)
  const to   = store.get(toId)
  if (!from || !to) return false
  if (from.septims < amount) return false

  const fromGold = from.septims - amount
  const toGold   = to.septims + amount

  store.update(fromId, { septims: fromGold })
  store.update(toId,   { septims: toGold })

  // Sync to inventory gold
  safeSet(mp, from.actorId, 'inv', _setGoldInInventory(safeGet<Inventory | null>(mp, from.actorId, 'inv', null), fromGold))
  safeSet(mp, to.actorId,   'inv', _setGoldInInventory(safeGet<Inventory | null>(mp, to.actorId,   'inv', null), toGold))

  return true
}

// ── Internal ──────────────────────────────────────────────────────────────────

const GOLD_BASE_ID = 0x0000000F

function _getGoldFromInventory(inv: Inventory | null): number {
  if (!inv || !inv.entries) return 0
  const entry = inv.entries.find(e => e.baseId === GOLD_BASE_ID)
  return entry ? entry.count : 0
}

function _setGoldInInventory(inv: Inventory | null, amount: number): Inventory {
  const entries = (inv && inv.entries) ? inv.entries.filter(e => e.baseId !== GOLD_BASE_ID) : []
  if (amount > 0) entries.push({ baseId: GOLD_BASE_ID, count: amount })
  return { entries }
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[economy] Initializing')

  const scheduleTick = () => {
    setTimeout(() => {
      try {
        for (const player of store.getAll()) {
          if (shouldPayStipend(player.minutesOnline, player.stipendPaidHours) && player.actorId) {
            const newSeptims = player.septims + STIPEND_RATE
            const newHours   = player.stipendPaidHours + 1
            store.update(player.id, { septims: newSeptims, stipendPaidHours: newHours })
            const inv = safeGet<Inventory | null>(mp, player.actorId, 'inv', null)
            safeSet(mp, player.actorId, 'inv', _setGoldInInventory(inv, newSeptims))
            safeSet(mp, player.actorId, 'ff_stipendHours', newHours)
            bus.dispatch({ type: 'stipendTick', playerId: player.id, septims: newSeptims, stipendPaidHours: newHours })
          }
        }
      } catch (err: any) {
        console.error(`[economy] Tick error: ${err.message}`)
      }
      scheduleTick()
    }, TICK_INTERVAL_MS)
  }

  scheduleTick()
  console.log('[economy] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player) return
  const inv  = safeGet<Inventory | null>(mp, player.actorId, 'inv', null)
  const gold = _getGoldFromInventory(inv)
  store.update(userId, { septims: gold })

  const hours = safeGet<number>(mp, player.actorId, 'ff_stipendHours', 0)
  store.update(userId, { stipendPaidHours: hours })
}
