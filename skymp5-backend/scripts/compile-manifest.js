'use strict'

/**
 * Compile an install manifest from a reference MO2 install.
 * 
 * Author overrides live in data/manifest-sources.json (all optional):
 *   {
 *     "urls":        { "<archiveName>": "https://direct-download/…" },
 *     "rootInclude": ["skse64_loader.exe", "IpHlpAPI.dll", …]
 *   }
 * `urls` gives a download source to non-Nexus archives; `rootInclude` lists
 * game-root files to capture (skse64_*.exe/.dll are picked up automatically).
 */

const fs      = require('fs')
const path    = require('path')
const crypto  = require('crypto')
const zlib    = require('zlib')
const { execFileSync } = require('child_process')
const SEVEN   = require('7zip-bin').path7za

// ── Args ──────────────────────────────────────────────────────────────────────

function parseArgs(argv) {
  const a = { profile: 'SkyRP' }
  for (let i = 0; i < argv.length; i++) {
    const k = argv[i]
    if      (k === '--mo2')     a.mo2     = argv[++i]
    else if (k === '--game')    a.game    = argv[++i]
    else if (k === '--profile') a.profile = argv[++i]
    else if (k === '--out')     a.out     = argv[++i]
  }
  return a
}

const args = parseArgs(process.argv.slice(2))
if (!args.mo2) {
  console.error('Usage: node scripts/compile-manifest.js --mo2 <MO2 root> [--game <game root>] [--profile SkyRP]')
  process.exit(1)
}

const MO2         = path.resolve(args.mo2)
const DOWNLOADS   = path.join(MO2, 'downloads')
const MODS        = path.join(MO2, 'mods')
const PROFILE_DIR = path.join(MO2, 'profiles', args.profile)
const DATA_DIR    = path.join(__dirname, '..', 'data')
const OUT         = args.out ? path.resolve(args.out) : path.join(DATA_DIR, 'install-manifest.json')
const MODLIST_OUT = path.join(DATA_DIR, 'modlist.json')

const INLINE_WARN = 50 * 1024 * 1024   // warn when inlining anything this large

let sources = { urls: {}, rootInclude: [] }
try {
  sources = { urls: {}, rootInclude: [], ...JSON.parse(fs.readFileSync(path.join(DATA_DIR, 'manifest-sources.json'), 'utf8')) }
} catch { /* optional */ }

// ── Hash helpers ──────────────────────────────────────────────────────────────

function sha256Buf(buf)  { return crypto.createHash('sha256').update(buf).digest('hex') }
function crc32hex(buf)   { return (zlib.crc32(buf) >>> 0).toString(16).toUpperCase().padStart(8, '0') }

function sha256File(p) {
  return new Promise((resolve, reject) => {
    const h = crypto.createHash('sha256')
    fs.createReadStream(p)
      .on('data', d => h.update(d))
      .on('end', () => resolve(h.digest('hex')))
      .on('error', reject)
  })
}

// ── FS helpers ────────────────────────────────────────────────────────────────

/** Recursively list files under dir as forward-slash paths relative to base. */
function walk(dir, base = dir, out = []) {
  for (const e of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, e.name)
    if (e.isDirectory()) walk(full, base, out)
    else out.push(path.relative(base, full).split(path.sep).join('/'))
  }
  return out
}

/** Read a download's .meta sidecar for Nexus mod/file ids. */
function readDownloadMeta(name) {
  try {
    const meta   = fs.readFileSync(path.join(DOWNLOADS, name + '.meta'), 'utf8')
    const modId  = (meta.match(/^modID\s*=\s*(\d+)/im)  || [])[1]
    const fileId = (meta.match(/^fileID\s*=\s*(\d+)/im) || [])[1]
    return { modId: modId ? Number(modId) : 0, fileId: fileId ? Number(fileId) : 0 }
  } catch { return { modId: 0, fileId: 0 } }
}

/** Read a mod folder's MO2 meta.ini for its Nexus mod id. */
function readModId(modDir) {
  try {
    const id = (fs.readFileSync(path.join(modDir, 'meta.ini'), 'utf8').match(/^modid\s*=\s*(\d+)/im) || [])[1]
    return id ? Number(id) : 0
  } catch { return 0 }
}

/** List archive entries as [{ path, size, crc }] (files only, with a CRC). */
function listEntries(archivePath) {
  const out = execFileSync(SEVEN, ['l', '-slt', '-ba', archivePath], {
    encoding: 'utf8',
    maxBuffer: 256 * 1024 * 1024,
    timeout: 5 * 60 * 1000,
  })
  const entries = []
  let cur = null
  const push = () => { if (cur && cur.path) entries.push(cur) }
  for (const line of out.split(/\r?\n/)) {
    if (line.startsWith('Path = '))        { push(); cur = { path: line.slice(7), size: 0, crc: '', folder: false } }
    else if (cur && line.startsWith('Size = '))   cur.size   = parseInt(line.slice(7), 10) || 0
    else if (cur && line.startsWith('CRC = '))    cur.crc    = line.slice(6).trim()
    else if (cur && line.startsWith('Folder = ')) cur.folder = line.slice(9).trim() === '+'
  }
  push()
  return entries
    .filter(e => e.crc && !e.folder)
    .map(e => ({ path: e.path.split('\\').join('/'), size: e.size, crc: e.crc }))
}

// ── Main ──────────────────────────────────────────────────────────────────────

async function main() {
  if (!fs.existsSync(MODS)) throw new Error(`mods folder not found: ${MODS}`)

  // ── 1. Index every archive's entries by (size, CRC32) ──────────────────────
  const archives = []                 // { id, hash, size, name, source, _entries }
  const index    = new Map()          // "size:CRC" -> { id, from }
  const referenced = new Set()

  const dlNames = fs.existsSync(DOWNLOADS)
    ? fs.readdirSync(DOWNLOADS).filter(n => !/\.(meta|unfinished)$/i.test(n))
    : []

  for (const name of dlNames) {
    const full = path.join(DOWNLOADS, name)
    let st
    try { st = fs.statSync(full) } catch { continue }
    if (!st.isFile()) continue

    let entries
    try { entries = listEntries(full) } catch { continue }   // not an archive
    if (entries.length === 0) continue

    const meta = readDownloadMeta(name)
    let source
    if (meta.modId && meta.fileId) source = { type: 'nexus', modId: meta.modId, fileId: meta.fileId }
    else if (sources.urls[name])   source = { type: 'url', url: sources.urls[name] }
    else                           source = { type: 'manual', name }

    const id   = 'a' + (archives.length + 1)
    const hash = await sha256File(full)
    archives.push({ id, hash, size: st.size, name, source })

    for (const e of entries) {
      const key = e.size + ':' + e.crc
      if (!index.has(key)) index.set(key, { id, from: e.path })   // first archive wins
    }
    console.log(`  indexed ${name} (${entries.length} entries, ${source.type})`)
  }

  // ── 2. Resolve the enabled mod order from the profile ──────────────────────
  let order = []
  try {
    order = fs.readFileSync(path.join(PROFILE_DIR, 'modlist.txt'), 'utf8')
      .split(/\r?\n/)
      .filter(l => l.startsWith('+'))
      .map(l => l.slice(1).trim())
      .filter(n => n && !n.endsWith('_separator'))
  } catch { /* no profile — fall back to every folder below */ }

  if (order.length === 0) {
    order = fs.readdirSync(MODS, { withFileTypes: true }).filter(e => e.isDirectory()).map(e => e.name)
    console.warn(`No profiles/${args.profile}/modlist.txt found — using all ${order.length} mod folders (unordered).`)
  }

  // ── 3. Emit a directive per file in each mod folder ────────────────────────
  const mods = []
  const inlineWarnings = []

  function directiveFor(absFile, toRel) {
    const buf = fs.readFileSync(absFile)
    const sha = sha256Buf(buf)
    const hit = index.get(buf.length + ':' + crc32hex(buf))
    if (hit) {
      referenced.add(hit.id)
      return { to: toRel, archive: hit.id, from: hit.from, sha256: sha, size: buf.length }
    }
    if (buf.length > INLINE_WARN) inlineWarnings.push(`${toRel} (${(buf.length / 1048576).toFixed(0)} MB)`)
    return { to: toRel, inline: buf.toString('base64'), sha256: sha, size: buf.length }
  }

  for (const modName of order) {
    const modDir = path.join(MODS, modName)
    if (!fs.existsSync(modDir)) continue
    const rels = walk(modDir).filter(r => r.toLowerCase() !== 'meta.ini')
    if (rels.length === 0) continue

    const files = rels.map(rel => directiveFor(path.join(modDir, rel.split('/').join(path.sep)), rel))
    mods.push({ name: modName, modId: readModId(modDir), files })
  }

  // ── 4. Optional game-root files (SKSE, preloaders) ─────────────────────────
  const root = []
  if (args.game) {
    const gameRoot = path.resolve(args.game)
    const rootRels = new Set(sources.rootInclude || [])
    try {
      for (const e of fs.readdirSync(gameRoot, { withFileTypes: true })) {
        if (!e.isDirectory() && /^skse64_.*\.(exe|dll)$/i.test(e.name)) rootRels.add(e.name)
      }
    } catch { /* unreadable game root */ }

    for (const rel of rootRels) {
      const full = path.join(gameRoot, rel.split('/').join(path.sep))
      if (!fs.existsSync(full)) { console.warn(`rootInclude not found, skipping: ${rel}`); continue }
      root.push(directiveFor(full, rel))
    }
  }

  // ── 5. Write the manifest (only referenced archives carry over) ────────────
  const usedArchives = archives
    .filter(a => referenced.has(a.id))
    .map(({ id, hash, size, name, source }) => ({ id, hash, size, name, source }))

  const manifest = {
    schema:  1,
    builtAt: new Date().toISOString(),
    game:    'skyrimspecialedition',
    archives: usedArchives,
    mods,
    root,
  }
  fs.mkdirSync(DATA_DIR, { recursive: true })
  fs.writeFileSync(OUT, JSON.stringify(manifest))

  // Lightweight display list so /api/modlist (the launcher's Modlist panel)
  // keeps its existing shape without a second source of truth.
  const display = [
    { name: 'SkyMP Client', required: true, enabled: true, source: 'backend' },
    ...mods.map(m => ({
      name: m.name, required: true, enabled: true,
      source: m.modId ? 'nexus' : 'url',
      ...(m.modId ? { nexusId: m.modId } : {}),
    })),
  ]
  fs.writeFileSync(MODLIST_OUT, JSON.stringify(display, null, 2) + '\n')

  // ── Report ─────────────────────────────────────────────────────────────────
  const inlineCount = mods.reduce((n, m) => n + m.files.filter(f => f.inline != null).length, 0) +
                      root.filter(f => f.inline != null).length
  console.log(`\narchives:    ${usedArchives.length} referenced (${archives.length} scanned)`)
  console.log(`mods:        ${mods.length}`)
  console.log(`root files:  ${root.length}`)
  console.log(`directives:  ${mods.reduce((n, m) => n + m.files.length, 0) + root.length} (${inlineCount} inline)`)

  const manual = usedArchives.filter(a => a.source.type === 'manual')
  if (manual.length) {
    console.warn('\nReferenced archives with NO download source — the launcher cannot fetch these.')
    console.warn('Add a URL for each in data/manifest-sources.json ("urls"):')
    for (const a of manual) console.warn(`  - ${a.name}`)
  }
  if (inlineWarnings.length) {
    console.warn('\nLarge files were inlined (bloats the manifest — add the source archive to downloads\\):')
    for (const w of inlineWarnings) console.warn(`  - ${w}`)
  }

  console.log(`\nWrote ${OUT}`)
  console.log(`Wrote ${MODLIST_OUT}`)
}

main().catch(err => {
  console.error('FAILED:', err.message)
  process.exit(1)
})
