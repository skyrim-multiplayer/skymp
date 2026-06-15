/**
 * Copies the built client files into the backend's file bucket
 *
 *   build/dist/client/Data/  →  <clientFilesDir>/root/Data/
 *
 * SKSE itself is not included here — the launcher installs it separately.
 *
 * Run from the backend/ directory:  npm run populate
 *   Override the source with SKYMP_CLIENT_DATA=<path to built Data/>.
 */

const fs   = require('fs')
const path = require('path')

// ── Source: the skymp build output Data/ directory ────────────────────────────
const SKYMP_DATA = process.env.SKYMP_CLIENT_DATA
  || path.join(__dirname, '..', '..', 'build', 'dist', 'client', 'Data')

// ── Destination ───────────────────────────────────────────────────────────────
const config    = require('../config')
const ROOT_DEST = path.join(config.clientFilesDir, 'root')
const DATA_DEST = path.join(ROOT_DEST, 'Data')

if (!fs.existsSync(SKYMP_DATA)) {
  console.error(`\nClient build output not found:\n  ${SKYMP_DATA}\n`)
  console.error('Build the client first, or set SKYMP_CLIENT_DATA to its Data/ folder.\n')
  process.exit(1)
}

// ── Copy the whole Data/ tree ─────────────────────────────────────────────────
let copied = 0
function copyTree(src, dest) {
  fs.mkdirSync(dest, { recursive: true })
  for (const entry of fs.readdirSync(src, { withFileTypes: true })) {
    const s = path.join(src, entry.name)
    const d = path.join(dest, entry.name)
    if (entry.isDirectory()) copyTree(s, d)
    else { fs.copyFileSync(s, d); copied++ }
  }
}

console.log(`\nCopying client Data from\n  ${SKYMP_DATA}\nto\n  ${DATA_DEST}`)
fs.rmSync(DATA_DEST, { recursive: true, force: true })
copyTree(SKYMP_DATA, DATA_DEST)

// ── Completeness check ────────────────────────────────────────────────────────
const REQUIRED = [
  'Platform/UI/index.html',                                   // CEF connect-window page
  'Platform/UI/build.js',                                     // connect-menu front-end bundle
  'Platform/Plugins/skymp5-client.js',                        // client logic
  'SKSE/Plugins/SkyrimPlatform.dll',                          // JS/CEF host plugin
  'SKSE/Plugins/MpClientPlugin.dll',                          // multiplayer plugin
  'Platform/Distribution/RuntimeDependencies/libcef.dll',     // CEF runtime
  'Platform/Distribution/RuntimeDependencies/SkyrimPlatformCEF.exe.hidden',
]
const missing = REQUIRED.filter(rel => !fs.existsSync(path.join(DATA_DEST, rel.replace(/\//g, path.sep))))

console.log(`\nDone. ${copied} file(s) copied.`)
if (missing.length > 0) {
  console.warn('\nWARNING — required client files are MISSING from the build output:')
  for (const m of missing) console.warn(`  - Data/${m}`)
  console.warn('The in-game client will not activate without them — rebuild the client.\n')
}
