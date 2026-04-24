// ── Courier ───────────────────────────────────────────────────────────────────

import { safeGet, safeSet } from '../../core/mpUtil'
import { signScript } from '../../core/signHelper'
import type { Mp, Store, Bus, Notification } from '../../types'

// ── Constants ─────────────────────────────────────────────────────────────────
const DEFAULT_EXPIRY_MS = 7 * 24 * 60 * 60 * 1000  // 7 days

// ── Pure helpers ──────────────────────────────────────────────────────────────

let _nextId = 1

export function createNotification(
  type: string,
  fromPlayerId: number,
  toPlayerId: number,
  holdId: string | null,
  payload: Record<string, unknown>,
  now?: number,
): Notification {
  const ts = now ?? Date.now()
  return {
    id: _nextId++,
    type,
    fromPlayerId,
    toPlayerId,
    holdId,
    payload,
    createdAt: ts,
    expiresAt: ts + DEFAULT_EXPIRY_MS,
    read: false,
  }
}

export function filterExpired(notifications: Notification[], now?: number): Notification[] {
  const ts = now ?? Date.now()
  return notifications.filter(n => n.expiresAt === null || ts < n.expiresAt)
}

export function getUnread(notifications: Notification[]): Notification[] {
  return notifications.filter(n => !n.read)
}

// ── Actions ───────────────────────────────────────────────────────────────────

export function sendNotification(mp: Mp, store: Store, notification: Notification): void {
  const recipient = store.get(notification.toPlayerId)
  if (!recipient || !recipient.actorId) return

  const existing = safeGet<Notification[]>(mp, recipient.actorId, 'ff_courier', [])
  const pruned = filterExpired(existing)
  pruned.push(notification)
  safeSet(mp, recipient.actorId, 'ff_courier', pruned)

  mp.sendCustomPacket(recipient.actorId, 'courierNotification', notification as unknown as Record<string, unknown>)
}

export function markRead(mp: Mp, store: Store, playerId: number, notificationId: number): void {
  const player = store.get(playerId)
  if (!player || !player.actorId) return
  const notes = safeGet<Notification[]>(mp, player.actorId, 'ff_courier', [])
  const updated = notes.map(n => n.id === notificationId ? Object.assign({}, n, { read: true }) : n)
  safeSet(mp, player.actorId, 'ff_courier', updated)
}

export function getPendingNotifications(mp: Mp, store: Store, playerId: number): Notification[] {
  const player = store.get(playerId)
  if (!player || !player.actorId) return []
  const notes = safeGet<Notification[]>(mp, player.actorId, 'ff_courier', [])
  return getUnread(filterExpired(notes))
}

// ── Init ─────────────────────────────────────────────────────────────────────

export function init(mp: Mp, store: Store, bus: Bus): void {
  console.log('[courier] Initializing')

  // Register ff_courier as a synced property so the FF plugin can receive
  // notification updates via the globalThis._ff.courier bridge.
  // The property value (Notification[]) is also persisted automatically.
  mp.makeProperty('ff_courier', {
    isVisibleByOwner:     true,
    isVisibleByNeighbors: false,
    updateOwner: signScript(
      'var ff=globalThis._ff;if(ff&&ff.courier)ff.courier.recv(ctx.value);'
    ),
    updateNeighbor: '',
  })

  console.log('[courier] Started')
}

export function onConnect(mp: Mp, store: Store, bus: Bus, userId: number): void {
  const player = store.get(userId)
  if (!player || !player.actorId) return
  const notes   = safeGet<Notification[]>(mp, player.actorId, 'ff_courier', [])
  const pending = getUnread(filterExpired(notes))
  for (const n of pending) {
    mp.sendCustomPacket(player.actorId, 'courierNotification', n as unknown as Record<string, unknown>)
  }
}
