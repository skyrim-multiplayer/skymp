'use strict'

// Remap load-order-dependent form IDs in the game server's changeForms store after the plugin load order changed.

const fs = require('fs')
const path = require('path')

function parseArgs(argv) {
  const a = { apply: false }
  for (let i = 0; i < argv.length; i++) {
    const k = argv[i]
    if      (k === '--dir')   a.dir = argv[++i]
    else if (k === '--old')   a.old = argv[++i]
    else if (k === '--new')   a.new = argv[++i]
    else if (k === '--apply') a.apply = true
  }
  return a
}

const args = parseArgs(process.argv.slice(2))
if (!args.dir || !args.old || !args.new) {
  console.error('Usage: node remap-changeforms.js --dir <changeForms> --old old.json --new new.json [--apply]')
  process.exit(1)
}

const readOrder = f => {
  const arr = JSON.parse(fs.readFileSync(f, 'utf8'))
  if (!Array.isArray(arr)) throw new Error(`${f} must be a JSON array of plugin names`)
  // Accept either bare names or full paths; index by lowercased basename.
  return arr.map(x => path.basename(String(x)).toLowerCase())
}

const oldOrder = readOrder(args.old)
const newOrder = readOrder(args.new)

// old index -> plugin name, plugin name -> new index
const oldName = oldOrder                                   // array: index -> name
const newIndex = new Map(newOrder.map((n, i) => [n, i]))

const idxOf = id => (id >>> 24) & 0xff
const localOf = id => id & 0x00ffffff
const VANILLA_MAX = 4

// Returns { id, note } - id remapped (or unchanged), note describes any issue.
function remapId(id) {
  if (typeof id !== 'number' || id <= 0 || id >= 0xff000000) return { id }
  const i = idxOf(id)
  if (i === 0xfe) return { id, note: `FE-space (light plugin) id 0x${(id>>>0).toString(16)} left unchanged - remap ESL ids by hand` }
  if (i <= VANILLA_MAX) return { id }                       // vanilla master, never moves
  const name = oldName[i]
  if (!name) return { id, note: `old index ${i} not in --old order (0x${(id>>>0).toString(16)}) left unchanged` }
  const ni = newIndex.get(name)
  if (ni === undefined) return { id, note: `plugin ${name} (old index ${i}) not in --new order - id 0x${(id>>>0).toString(16)} left unchanged` }
  if (ni === i) return { id }                               // no shift
  return { id: ((ni << 24) | localOf(id)) >>> 0, from: i, to: ni, name }
}

const changes = []
const notes = []
let filesTouched = 0

const files = fs.readdirSync(args.dir).filter(n => n.endsWith('.json'))
const patched = new Map()   // file -> new content string (only when something changed)

for (const f of files) {
  const full = path.join(args.dir, f)
  let cf
  try { cf = JSON.parse(fs.readFileSync(full, 'utf8')) } catch { continue }
  let changed = false

  const doId = (label, id) => {
    const r = remapId(id)
    if (r.note) notes.push(`${f}: ${label}: ${r.note}`)
    if (r.from !== undefined) {
      changes.push(`${f}: ${label} 0x${(id>>>0).toString(16)} -> 0x${(r.id>>>0).toString(16)} (${r.name}: idx ${r.from} -> ${r.to})`)
      changed = true
    }
    return r.id
  }

  const ap = cf.appearanceDump
  if (ap) {
    if (typeof ap.raceId === 'number') ap.raceId = doId('appearance.raceId', ap.raceId)
    if (Array.isArray(ap.headpartIds)) ap.headpartIds = ap.headpartIds.map((h, i) => doId(`appearance.headpart[${i}]`, h))
  }
  for (const e of (cf.inv && cf.inv.entries) || []) if (typeof e.baseId === 'number') e.baseId = doId('inv.baseId', e.baseId)
  const eq = cf.equipmentDump && cf.equipmentDump.inv
  for (const e of (eq && eq.entries) || []) if (typeof e.baseId === 'number') e.baseId = doId('equip.baseId', e.baseId)

  if (changed) { patched.set(full, JSON.stringify(cf, null, 2) + '\n'); filesTouched++ }
}

console.log(`changeForms: ${files.length} scanned, ${filesTouched} would change, ${changes.length} id(s) remapped`)
if (notes.length) {
  console.log(`\nLEFT UNCHANGED (review these - they may still be broken):`)
  for (const n of notes) console.log('  ' + n)
}
if (changes.length) {
  console.log(`\nREMAPS:`)
  for (const c of changes) console.log('  ' + c)
}

if (!args.apply) {
  console.log(`\nDRY RUN - nothing written. Re-run with --apply to write (a timestamped backup is made first).`)
  process.exit(0)
}

if (patched.size === 0) { console.log('\nNothing to write.'); process.exit(0) }

// Back up the whole changeForms folder before writing.
const stamp = new Date().toISOString().replace(/[:.]/g, '-')
const backup = args.dir.replace(/[\\/]+$/, '') + '.backup-' + stamp
fs.mkdirSync(backup, { recursive: true })
for (const f of files) fs.copyFileSync(path.join(args.dir, f), path.join(backup, f))
console.log(`\nBacked up ${files.length} file(s) to ${backup}`)

for (const [full, content] of patched) fs.writeFileSync(full, content)
console.log(`Wrote ${patched.size} changed file(s). STOP the game server before running this, then start it after.`)
