/**
 * Copies client files from the skymp build output into the backend's file bucket:
 *
 *   public/files/root/   → installed into {skyrimPath}/ root
 *     Data/              → sub-directory for all Data/ files
 *
 * SKSE is managed by Vortex and is NOT deployed here.
 *
 * Run from the backend/ directory:  npm run populate
 */

const fs   = require('fs')
const path = require('path')

// ── Source paths ──────────────────────────────────────────────────────────────

// skymp build output Data/ directory
const SKYMP_DATA = 'E:\\Github\\skymp\\build\\dist\\client\\Data'

// ── Destination ───────────────────────────────────────────────────────────────

const ROOT_DEST = path.join(__dirname, '..', 'public', 'files', 'root')
const DATA_DEST = path.join(ROOT_DEST, 'Data')

fs.mkdirSync(DATA_DEST, { recursive: true })

let copied  = 0
let missing = 0

// ── Helper ────────────────────────────────────────────────────────────────────

function copyEntry(srcAbs, destAbs, label) {
  const stat = fs.statSync(srcAbs, { throwIfNoEntry: false })
  if (!stat) {
    console.warn(`  MISSING  ${label}`)
    missing++
    return
  }
  if (stat.isDirectory()) {
    fs.cpSync(srcAbs, destAbs, { recursive: true })
    console.log(`  Copied   ${label}/`)
  } else {
    fs.mkdirSync(path.dirname(destAbs), { recursive: true })
    fs.copyFileSync(srcAbs, destAbs)
    console.log(`  Copied   ${label}`)
  }
  copied++
}

// ── 1. Data/ files (go to public/files/root/Data/) ───────────────────────────

console.log('\n── Data files ────────────────────────────────')

const DATA_FILES = [
  ['Platform\\Plugins\\skymp5-client.js',           'Platform/Plugins/skymp5-client.js'],
  ['Platform\\Plugins\\skymp5-client-settings.txt', 'Platform/Plugins/skymp5-client-settings.txt'],
  ['Platform\\Distribution',                         'Platform/Distribution'],
  ['SKSE\\Plugins\\SkyrimPlatform.dll',              'SKSE/Plugins/SkyrimPlatform.dll'],
  ['SKSE\\Plugins\\MpClientPlugin.dll',              'SKSE/Plugins/MpClientPlugin.dll'],
]

// .pex scripts — flat copy
const scriptsSrc  = path.join(SKYMP_DATA, 'Scripts')
const scriptsDest = path.join(DATA_DEST, 'Scripts')
fs.mkdirSync(scriptsDest, { recursive: true })
try {
  const pex = fs.readdirSync(scriptsSrc).filter(f => f.endsWith('.pex'))
  for (const f of pex) {
    fs.copyFileSync(path.join(scriptsSrc, f), path.join(scriptsDest, f))
    console.log(`  Copied   Scripts/${f}`)
    copied++
  }
} catch (e) {
  console.warn(`  WARNING  Cannot read Scripts/: ${e.message}`)
}

for (const [srcRel, destRel] of DATA_FILES) {
  copyEntry(
    path.join(SKYMP_DATA, srcRel),
    path.join(DATA_DEST, destRel.replace(/\//g, path.sep)),
    srcRel
  )
}

// ── Report ────────────────────────────────────────────────────────────────────

console.log(`\nDone. ${copied} item(s) copied, ${missing} missing.\n`)
