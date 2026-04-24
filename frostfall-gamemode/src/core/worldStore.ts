// ── World Store ───────────────────────────────────────────────────────────────
// File-backed key-value store for world-level data (properties, prison queue,
// faction docs). Avoids depending on any SkyMP form ID existing.
// Writes are synchronous to prevent partial-write corruption on crash.

import fs from 'fs'
import path from 'path'

const FILE = path.join(__dirname, '..', '..', 'world', 'ff-world-data.json')

type WorldData = Record<string, unknown>

let _cache: WorldData | null = null

function _load(): WorldData {
  if (_cache) return _cache
  try {
    _cache = JSON.parse(fs.readFileSync(FILE, 'utf8')) as WorldData
  } catch {
    _cache = {}
  }
  return _cache
}

function _save(): void {
  try {
    const dir = path.dirname(FILE)
    if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true })
    fs.writeFileSync(FILE, JSON.stringify(_cache, null, 2))
  } catch (err: any) {
    console.error('[worldStore] Failed to save world data:', err?.message ?? err)
  }
}

export function get<T = unknown>(key: string): T | null {
  const data = _load()
  return data[key] !== undefined ? data[key] as T : null
}

export function set(key: string, value: unknown): void {
  _load()
  _cache![key] = value
  _save()
}
