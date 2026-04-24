'use strict'
// Has to be refactored to work with the new ts system.
// Took from a Unity project and adapted for SkyMP. Still a WIP, but the core spawning/despawning logic is in place and can be tested with placeholder formIds. Once we have a way to reference Skyrim records, we'll need to update the formIds and verify that the spawns work as intended (e.g. spawn positions, facing angles, group sizes).
// ─────────────────────────────────────────────────────────────────────────────
// wildlife.js — server-side procedural wildlife for SkyMP
//
// Require from gamemode.js:
//   const wildlife = require('./wildlife')
//   wildlife.init(mp)
//
// Currently broken
// ─────────────────────────────────────────────────────────────────────────────

// ── Configuration ─────────────────────────────────────────────────────────────

const TICK_INTERVAL_MS        = 5_000        // how often the system updates
const ZONE_RADIUS             = 3_000        // spawn radius per player (Skyrim units, ~1u = 14cm)
const MERGE_THRESHOLD         = ZONE_RADIUS * 2  // two zones merge when centers are closer than this
const MAX_PER_ZONE            = 5            // max alive wildlife per merged zone
const DESPAWN_GRACE_MS        = 15_000       // keep creatures alive this long after a player leaves the area
const DEATH_COOLDOWN_MS       = 5 * 60_000   // after a creature dies, its species won't respawn in that worldspace for this long
const SPAWN_CHANCE            = 0.20         // probability per zone per tick that a spawn attempt is made
const ZONE_SPAWN_COOLDOWN_MS  = 60_000       // min time between spawn events in a given worldspace
const MIN_SPAWN_DIST          = 1_400        // don't spawn within this distance of any player (~200m, outside visual pop-in range)
const SPAWN_Z_OFFSET          = -256         // place actors this many units below the spawn point so Skyrim's ground-finding kicks in before the player sees them

// Note: mp.place() creates the actor at [0,0,0] and fires subscription updates
// before mp.set(locationalData) is called, so clients near the origin briefly
// see the actor there. SPAWN_Z_OFFSET puts new actors underground until Skyrim
// places them on the navmesh surface.

const TAMRIEL_ID = 0x0000003C

// ── Spawn table ───────────────────────────────────────────────────────────────
// formId: base NPC record in Skyrim.esm (verify with xEdit if a creature misbehaves)
// weight: relative spawn chance within any zone that lists this species
// minGroup / maxGroup: creatures spawned per spawn event

// formIds sourced from skyrim-esm-data/npcs.json export. Each entry notes its EditorID for cross-reference.
const SPAWN_TABLE = [
  // Wolves
  { id: 'wolf',        name: 'Wolf',        formId: 0x0010FE05, weight: 30, minGroup: 1, maxGroup: 2 }, // EncWolfRed
  { id: 'wolf_timber', name: 'Timber Wolf', formId: 0x00023ABF, weight: 15, minGroup: 1, maxGroup: 2 }, // EncWolfIce

  // Bears
  { id: 'bear_black',  name: 'Bear (Black)',formId: 0x00023A8B, weight: 10, minGroup: 1, maxGroup: 1 }, // EncBearCave
  { id: 'bear_brown',  name: 'Bear (Brown)',formId: 0x00023A8A, weight: 10, minGroup: 1, maxGroup: 1 }, // EncBear
  { id: 'bear_snow',   name: 'Bear (Snow)', formId: 0x00023A8C, weight:  5, minGroup: 1, maxGroup: 1 }, // EncBearSnow

  // Peaceful
  { id: 'deer',        name: 'Deer',        formId: 0x000CF89D, weight: 25, minGroup: 1, maxGroup: 3 }, // EncDeer
  { id: 'elk',         name: 'Elk',         formId: 0x00023A91, weight: 15, minGroup: 1, maxGroup: 2 }, // EncElk

  // Small
  { id: 'mudcrab',     name: 'Mudcrab',     formId: 0x000E4010, weight: 20, minGroup: 1, maxGroup: 2 }, // EncMudcrabMedium
  { id: 'skeever',     name: 'Skeever',     formId: 0x00023AB7, weight: 20, minGroup: 1, maxGroup: 2 }, // EncSkeever

  // Apex
  { id: 'sabrecat',    name: 'Sabre Cat',   formId: 0x00023AB5, weight:  8, minGroup: 1, maxGroup: 1 }, // EncSabreCat
  { id: 'horker',      name: 'Horker',      formId: 0x00023AB1, weight: 10, minGroup: 1, maxGroup: 3 }, // EncHorker
]

// ── Spawn zones ───────────────────────────────────────────────────────────────
// WoW-style geographic regions. Each defines the creature roster for that area.
// Zones are checked in order — first bounds match wins.
// Only applies to the Tamriel worldspace (0x0000003C); other worldspaces use 'default'.
//
// Coordinates are approximate Skyrim worldspace units (~14cm per unit).
// Use xEdit's View → Set Active Cell to verify positions if in doubt.
const SPAWN_ZONES = [
  // ── Far north — coastline along the Sea of Ghosts ─────────────────────────
  {
    name: 'Northern Coastline',
    bounds: { minX: -55000, maxX: 70000, minY: 43000, maxY: 70000 },
    species: ['horker', 'wolf_timber', 'bear_snow'],
  },

  // ── Northeast — Winterhold and surrounding tundra ─────────────────────────
  {
    name: 'Winterhold',
    bounds: { minX: 28000, maxX: 70000, minY: 28000, maxY: 43000 },
    species: ['wolf_timber', 'bear_snow', 'wolf', 'skeever'],
  },

  // ── North-central — The Pale (Dawnstar hold) ──────────────────────────────
  {
    name: 'The Pale',
    bounds: { minX: -20000, maxX: 28000, minY: 28000, maxY: 43000 },
    species: ['wolf', 'wolf_timber', 'bear_snow', 'deer'],
  },

  // ── Northwest — Haafingar (Solitude hold) ────────────────────────────────
  {
    name: 'Haafingar',
    bounds: { minX: -55000, maxX: -15000, minY: 28000, maxY: 43000 },
    species: ['wolf', 'bear_brown', 'bear_snow', 'deer', 'elk'],
  },

  // ── West-central — Hjaalmarch (Morthal, swampy marshland) ─────────────────
  {
    name: 'Hjaalmarch',
    bounds: { minX: -40000, maxX: -5000, minY: 10000, maxY: 28000 },
    species: ['skeever', 'mudcrab', 'wolf', 'bear_black'],
  },

  // ── East — Eastmarch (Windhelm hold, volcanic tundra) ────────────────────
  {
    name: 'Eastmarch',
    bounds: { minX: 28000, maxX: 65000, minY: 5000, maxY: 28000 },
    species: ['wolf', 'bear_brown', 'skeever', 'deer'],
  },

  // ── Centre — Whiterun Hold (open tundra plains) ───────────────────────────
  {
    name: 'Whiterun Hold',
    bounds: { minX: -10000, maxX: 28000, minY: -15000, maxY: 20000 },
    species: ['wolf', 'sabrecat', 'deer', 'elk', 'mudcrab'],
  },

  // ── West — The Reach (rocky canyons, Markarth hold) ──────────────────────
  {
    name: 'The Reach',
    bounds: { minX: -70000, maxX: -25000, minY: -30000, maxY: 28000 },
    species: ['wolf', 'bear_brown', 'bear_black', 'sabrecat'],
  },

  // ── Southeast — The Rift (Riften hold, autumnal forest) ──────────────────
  {
    name: 'The Rift',
    bounds: { minX: 20000, maxX: 65000, minY: -40000, maxY: 5000 },
    species: ['wolf', 'bear_black', 'skeever', 'deer', 'elk', 'sabrecat'],
  },

  // ── South — Falkreath Hold (dense pine forest) ───────────────────────────
  {
    name: 'Falkreath Hold',
    bounds: { minX: -25000, maxX: 20000, minY: -55000, maxY: -15000 },
    species: ['wolf', 'bear_black', 'bear_brown', 'deer', 'elk', 'mudcrab'],
  },

  // ── Fallback — any unmatched position (gaps, border areas) ───────────────
  {
    name: 'default',
    bounds: null,
    species: ['wolf', 'deer', 'elk', 'mudcrab', 'skeever'],
  },
]

// Returns the SPAWN_ZONES entry whose bounds contain the zone centre.
// Falls back to the 'default' entry for unmatched positions and non-Tamriel worldspaces.
function getSpawnZone(zone) {
  if (zone.cellOrWorld === TAMRIEL_ID) {
    const [x, y] = zone.center
    const match = SPAWN_ZONES.find(z =>
      z.bounds &&
      x >= z.bounds.minX && x <= z.bounds.maxX &&
      y >= z.bounds.minY && y <= z.bounds.maxY
    )
    if (match) return match
  }
  return SPAWN_ZONES.find(z => z.name === 'default')
}

// Weighted random pick from an arbitrary subset of SPAWN_TABLE entries
function pickWeightedRandom(entries) {
  let r = Math.random() * entries.reduce((s, e) => s + e.weight, 0)
  for (const entry of entries) {
    r -= entry.weight
    if (r <= 0) return entry
  }
  return entries[entries.length - 1]
}

// ── State ─────────────────────────────────────────────────────────────────────

// Tracks every actor we spawned
// formId → { entryId, pos: [x,y,z], cellOrWorld: number, spawnedAt: number }
const spawnedActors = new Map()

// Key: `${entryId}:${cellOrWorldId}` → ms timestamp after which respawn is allowed
const deathCooldowns = new Map()

// formId → ms timestamp after which the actor may be despawned (grace window)
const despawnGraceMap = new Map()

// Connected userIds — populated via mp.on("connect"/"disconnect")
const onlineUserIds = new Set()

// cellOrWorld → ms timestamp after which a new spawn event is allowed in that worldspace
const zoneSpawnCooldowns = new Map()

// ── Math helpers ──────────────────────────────────────────────────────────────

function dist3D(a, b) {
  const dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2]
  return Math.sqrt(dx*dx + dy*dy + dz*dz)
}

function randInt(min, max) {
  return Math.floor(Math.random() * (max - min + 1)) + min
}

function randAngle() {
  return Math.random() * Math.PI * 2
}

// Uniformly distributed random point inside a circle (2D, Z kept from center)
function randPosInCircle(center, radius) {
  const angle = randAngle()
  const r     = Math.sqrt(Math.random()) * radius * 0.85 // 0.85 keeps spawns away from the very edge
  return [
    center[0] + Math.cos(angle) * r,
    center[1] + Math.sin(angle) * r,
    center[2],
  ]
}

// Find a spawn position that is at least MIN_SPAWN_DIST away from every player.
// Returns null if no valid position is found after maxAttempts tries.
function findSpawnPos(zone, players, maxAttempts = 10) {
  for (let i = 0; i < maxAttempts; i++) {
    const pos = randPosInCircle(zone.center, zone.radius)
    if (players.every(p => dist3D(pos, p.pos) >= MIN_SPAWN_DIST)) return pos
  }
  return null
}

// ── Zone merging ──────────────────────────────────────────────────────────────
//
// Algorithm:
//   1. Group players by worldspace (different worlds never merge)
//   2. Build an adjacency graph: two players are adjacent if dist < MERGE_THRESHOLD
//   3. Find connected components via BFS
//   4. Each component becomes one zone whose bounding sphere wraps all its players
//
// Result: O(n²) over players (≤100), called every 5s — negligible cost.

function buildZones(players) {
  if (players.length === 0) return []

  // Group by worldspace
  const byWorld = new Map()
  players.forEach((p, i) => {
    if (!byWorld.has(p.cellOrWorld)) byWorld.set(p.cellOrWorld, [])
    byWorld.get(p.cellOrWorld).push(i)
  })

  const zones = []

  for (const [worldId, indices] of byWorld) {
    const n   = indices.length
    const adj = Array.from({ length: n }, () => [])

    for (let a = 0; a < n; a++) {
      for (let b = a + 1; b < n; b++) {
        if (dist3D(players[indices[a]].pos, players[indices[b]].pos) < MERGE_THRESHOLD) {
          adj[a].push(b)
          adj[b].push(a)
        }
      }
    }

    // BFS connected components
    const visited = new Set()
    for (let start = 0; start < n; start++) {
      if (visited.has(start)) continue

      const cluster = []
      const queue   = [start]
      visited.add(start)

      while (queue.length) {
        const curr = queue.shift()
        cluster.push(indices[curr])
        for (const next of adj[curr]) {
          if (!visited.has(next)) { visited.add(next); queue.push(next) }
        }
      }

      // Bounding sphere: center = mean of player positions
      // effective radius = (max distance from center to any player) + ZONE_RADIUS
      const positions = cluster.map(i => players[i].pos)
      const cx = positions.reduce((s, p) => s + p[0], 0) / positions.length
      const cy = positions.reduce((s, p) => s + p[1], 0) / positions.length
      const cz = positions.reduce((s, p) => s + p[2], 0) / positions.length
      const center = [cx, cy, cz]
      const spread = positions.reduce((max, p) => Math.max(max, dist3D(center, p)), 0)

      zones.push({
        center,
        radius:       spread + ZONE_RADIUS,
        cellOrWorld:  worldId,
        playerCount:  cluster.length,
      })
    }
  }

  return zones
}

function isInZone(pos, zone) {
  return dist3D(pos, zone.center) <= zone.radius
}

function isInAnyZone(pos, zones) {
  return zones.some(z => isInZone(pos, z))
}

// ── Spawn tick ────────────────────────────────────────────────────────────────

function tick(mp) {
  const now = Date.now()

  // 1. Collect online player positions
  const players = []

  for (const userId of onlineUserIds) {
    try {
      const actorId    = mp.getUserActor(userId)
      if (!actorId) continue
      const pos        = mp.getActorPos(actorId)
      const cellOrWorld = mp.getActorCellOrWorld(actorId)
      if (pos && cellOrWorld) players.push({ actorId, pos, cellOrWorld })
    } catch {
      // Player may have just disconnected — skip silently
    }
  }

  // 2. Build merged zones
  const zones = buildZones(players)

  // 3. Despawn actors that are no longer covered by any zone
  for (const [formId, info] of spawnedActors) {
    if (isInAnyZone(info.pos, zones)) {
      // Still covered — cancel any pending grace timer
      despawnGraceMap.delete(formId)
      continue
    }

    if (!despawnGraceMap.has(formId)) {
      // Start grace period — don't despawn immediately in case player turns around
      despawnGraceMap.set(formId, now + DESPAWN_GRACE_MS)
    } else if (now >= despawnGraceMap.get(formId)) {
      // Grace expired — despawn
      try { mp.destroyActor(formId) } catch {}
      spawnedActors.delete(formId)
      despawnGraceMap.delete(formId)
    }
  }

  // 4. Spawn wildlife into each zone up to MAX_PER_ZONE
  for (const zone of zones) {
    // Gate 1: per-worldspace spawn cooldown — prevents burst-filling after a player joins
    const spawnCooldownExpiry = zoneSpawnCooldowns.get(zone.cellOrWorld)
    if (spawnCooldownExpiry && now < spawnCooldownExpiry) continue

    // Gate 2: random chance — only a fraction of ticks actually trigger a spawn attempt
    if (Math.random() > SPAWN_CHANCE) continue

    // Gate 3: zone already at capacity
    let aliveCount = 0
    for (const [, info] of spawnedActors) {
      if (isInZone(info.pos, zone)) aliveCount++
    }
    if (aliveCount >= MAX_PER_ZONE) continue

    // Resolve the geographic spawn zone and build the eligible species list
    const spawnZone = getSpawnZone(zone)
    const eligible  = SPAWN_TABLE.filter(e => spawnZone.species.includes(e.id))
    if (eligible.length === 0) continue

    // Pick a random species from eligible entries, respecting death cooldowns
    let entry = null
    for (let attempt = 0; attempt < eligible.length; attempt++) {
      const candidate   = pickWeightedRandom(eligible)
      const cooldownKey = `${candidate.id}:${zone.cellOrWorld}`
      if (!deathCooldowns.has(cooldownKey) || now >= deathCooldowns.get(cooldownKey)) {
        entry = candidate
        break
      }
    }
    if (!entry) continue // all eligible species on cooldown in this worldspace

    const groupSize = Math.min(
      randInt(entry.minGroup, entry.maxGroup),
      MAX_PER_ZONE - aliveCount
    )

    let spawned = 0
    for (let g = 0; g < groupSize; g++) {
      // Only spawn at positions outside player visual range
      const spawnPos = findSpawnPos(zone, players)
      if (!spawnPos) break // couldn't find a safe position, abort this group

      try {
        // mp.place(baseFormId) creates an actor from the NPC_ ESM record and
        // auto-assigns a dynamic formId (0xff000000+). mp.createActor() assigns
        // a formId directly, which conflicts with ESM records — don't use it for wildlife.
        const newId = mp.place(entry.formId)
        if (newId) {
          // SPAWN_Z_OFFSET shifts the actor underground at the spawn position.
          // mp.place() fires subscription updates before this set() call, so
          // without the offset clients briefly see the actor at [0,0,0]. The
          // negative Z puts it below the surface; Skyrim's ground-placement
          // moves it up to the navmesh when the cell loads.
          mp.set(newId, 'locationalData', {
            pos:             [spawnPos[0], spawnPos[1], spawnPos[2] + SPAWN_Z_OFFSET],
            rot:             [0, 0, randAngle()],
            cellOrWorldDesc: mp.getDescFromId(zone.cellOrWorld),
          })
          spawnedActors.set(newId, {
            entryId:     entry.id,
            pos:         spawnPos,
            cellOrWorld: zone.cellOrWorld,
            spawnedAt:   now,
          })
          spawned++
        }
      } catch (err) {
        console.error(`[wildlife] place failed for ${entry.name} (0x${entry.formId.toString(16)}): ${err.message}`)
      }
    }

    if (spawned > 0) {
      // Lock this worldspace from spawning again for a while
      zoneSpawnCooldowns.set(zone.cellOrWorld, now + ZONE_SPAWN_COOLDOWN_MS)
    }
  }
}

// ── Public init ───────────────────────────────────────────────────────────────

function init(mp) {
  console.log('[wildlife] Initializing')

  // Track connected players — mp.on("connect"/"disconnect")
  mp.on('connect', (userId) => onlineUserIds.add(userId))
  mp.on('disconnect', (userId) => onlineUserIds.delete(userId))

  // Detect wildlife deaths via a client-side event source.
  // actorKill fires on the client whenever any actor is killed nearby.
  // The server receives the formId of the player whose client observed the kill.
  mp.makeEventSource('_onActorDeath', `
    ctx.sp.on('actorKill', () => {
      ctx.sendEvent();
    });
  `)
  mp._onActorDeath = (pcFormId) => {
    const worldspace = mp.getActorCellOrWorld(pcFormId)
    for (const [id, info] of spawnedActors) {
      if (info.cellOrWorld !== worldspace) continue
      try { mp.getActorPos(id) } catch {
        // Actor is gone — apply cooldown and stop tracking it
        const cooldownKey = `${info.entryId}:${info.cellOrWorld}`
        deathCooldowns.set(cooldownKey, Date.now() + DEATH_COOLDOWN_MS)
        spawnedActors.delete(id)
        despawnGraceMap.delete(id)
        console.log(`[wildlife] ${info.entryId} gone (id=0x${id.toString(16)}), cooldown ${DEATH_COOLDOWN_MS / 1000}s`)
      }
    }
  }

  const scheduleTick = () => {
    setTimeout(() => {
      try { tick(mp) } catch (err) {
        console.error(`[wildlife] Tick error: ${err.message}`)
      }
      scheduleTick()
    }, TICK_INTERVAL_MS)
  }

  scheduleTick()

  console.log(
    `[wildlife] Started — ${SPAWN_TABLE.length} species, zone radius ${ZONE_RADIUS}u, ` +
    `max ${MAX_PER_ZONE}/zone, tick ${TICK_INTERVAL_MS}ms`
  )
}

module.exports = { init }
