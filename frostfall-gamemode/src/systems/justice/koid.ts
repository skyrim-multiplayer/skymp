// ── KOID Table ────────────────────────────────────────────────────────────────
// Kill-On-Identification: which faction pairs have mutual lethal-force permission.
// This is a reference table only — it does not enforce anything on its own.

interface KoidPair {
  factionA: string
  factionB: string
  reason: string
}

const KOID_PAIRS: KoidPair[] = [
  { factionA: 'thalmor',          factionB: 'stormcloakUnderground', reason: 'Standing Justiciar orders' },
  { factionA: 'imperialGarrison', factionB: 'stormcloakUnderground', reason: 'Active conflict' },
  { factionA: 'guard',            factionB: 'highBounty',            reason: 'Wanted criminal threshold' },
]

export function hasKoidPermission(factionA: string, factionB: string): boolean {
  return KOID_PAIRS.some(p =>
    (p.factionA === factionA && p.factionB === factionB) ||
    (p.factionA === factionB && p.factionB === factionA)
  )
}

export function getKoidPair(factionA: string, factionB: string): KoidPair | null {
  return KOID_PAIRS.find(p =>
    (p.factionA === factionA && p.factionB === factionB) ||
    (p.factionA === factionB && p.factionB === factionA)
  ) ?? null
}

export function getKoidTargeters(faction: string): string[] {
  const result: string[] = []
  for (const p of KOID_PAIRS) {
    if (p.factionA === faction) result.push(p.factionB)
    if (p.factionB === faction) result.push(p.factionA)
  }
  return result
}
