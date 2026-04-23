// ── Event Bus ─────────────────────────────────────────────────────────────────
// Minimal event emitter for inter-system communication.
// Systems never call each other directly — they dispatch events and listen.

import type { BusEvent, Bus } from '../types'

type Handler = (event: BusEvent) => void

const handlers = new Map<string, Handler[]>()

function on(type: string, fn: Handler): void {
  if (!handlers.has(type)) handlers.set(type, [])
  handlers.get(type)!.push(fn)
}

function off(type: string, fn: Handler): void {
  if (!handlers.has(type)) return
  const list = handlers.get(type)!.filter(h => h !== fn)
  handlers.set(type, list)
}

function dispatch(event: BusEvent): void {
  const list = handlers.get(event.type)
  if (!list) return
  for (const fn of list) {
    try { fn(event) } catch (err: any) {
      console.error(`[bus] Handler error for "${event.type}": ${err.message}`)
    }
  }
}

export const bus: Bus = { on, off, dispatch }
