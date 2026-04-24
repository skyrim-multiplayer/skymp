// ── Safe mp wrappers ──────────────────────────────────────────────────────────
//
// PartOne::GetUserActor returns 0 when no actor has been assigned yet (a "no
// actor" sentinel).  The C++ form with id 0x0 never exists in worldState at
// connect-time, so any mp.get / mp.set call with actorId 0 (or any id whose
// form isn't loaded yet) throws "Form with id 0x0 doesn't exist" and produces
// ANTIGO context noise in the server log.
//
// Use these wrappers everywhere a module reads or writes a custom ff_* property
// so that a not-yet-ready actor is silently skipped instead of erroring.

import type { Mp } from '../types'

export function safeGet<T>(mp: Mp, actorId: number, key: string, fallback: T): T {
  if (!actorId) return fallback
  try {
    const val = mp.get(actorId, key)
    return (val !== null && val !== undefined) ? val as T : fallback
  } catch {
    return fallback
  }
}

export function safeSet(mp: Mp, actorId: number, key: string, value: unknown): boolean {
  if (!actorId) return false
  try {
    mp.set(actorId, key, value)
    return true
  } catch {
    return false
  }
}
