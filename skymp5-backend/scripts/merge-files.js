'use strict'

/**
 * Merge pipeline — copies the client source directory into the file bucket
 * that the launcher downloads, and builds the distributable zip.
 *
 *   sources/client/  →  public/files/root/
 *                    →  public/files/frostfall-client.zip
 *                    →  data/files-version.json
 *
 * SKSE is NOT included here — it is managed by the user via the Vortex collection.
 *
 * Run standalone:  node scripts/merge-files.js
 * Called by:       scripts/setup-client.js  and  routes/webhook.js
 */

const path               = require('path')
const fs                 = require('fs')
const { execFileSync }   = require('child_process')
const archiver           = require('archiver')

const ROOT = path.join(__dirname, '..')

const CLIENT_SRC   = path.join(ROOT, 'sources', 'client')
const OUTPUT_DIR   = path.join(ROOT, 'public', 'files', 'root')
const ZIP_PATH     = path.join(ROOT, 'public', 'files', 'frostfall-client.zip')
const VERSION_FILE = path.join(ROOT, 'data', 'files-version.json')

// ── Version helpers ───────────────────────────────────────────────────────────

/**
 * Short git commit hash of the client source repo.
 * This changes exactly when new commits are pulled — never on mere restarts.
 * Falls back to 'nogit' if the directory isn't a repo yet.
 */
function clientGitHash() {
  try {
    return execFileSync('git', ['-C', CLIENT_SRC, 'rev-parse', '--short', 'HEAD'], {
      encoding: 'utf8',
      stdio:    ['ignore', 'pipe', 'ignore'],
    }).trim()
  } catch {
    return 'nogit'
  }
}

// ── File copy ─────────────────────────────────────────────────────────────────

function copyDir(srcDir, destDir, skipNames = new Set()) {
  if (!fs.existsSync(srcDir)) {
    console.warn(`[merge] source not found, skipping: ${srcDir}`)
    return 0
  }
  let count = 0
  for (const entry of fs.readdirSync(srcDir, { withFileTypes: true })) {
    if (skipNames.has(entry.name)) continue
    const src  = path.join(srcDir, entry.name)
    const dest = path.join(destDir, entry.name)
    if (entry.isDirectory()) {
      fs.mkdirSync(dest, { recursive: true })
      count += copyDir(src, dest, skipNames)
    } else {
      fs.mkdirSync(path.dirname(dest), { recursive: true })
      fs.copyFileSync(src, dest)
      count++
    }
  }
  return count
}

// ── Zip builder ───────────────────────────────────────────────────────────────

function buildZip(srcDir, zipPath) {
  return new Promise((resolve, reject) => {
    fs.mkdirSync(path.dirname(zipPath), { recursive: true })
    const output  = fs.createWriteStream(zipPath)
    const archive = archiver('zip', { zlib: { level: 6 } })

    output.on('close', () => resolve(archive.pointer()))
    archive.on('error', reject)

    archive.pipe(output)
    archive.directory(srcDir, false)  // false = no root folder prefix in zip
    archive.finalize()
  })
}

// ── Main export ───────────────────────────────────────────────────────────────

async function mergeSourcesIntoRoot() {
  const startMs = Date.now()

  console.log('[merge] Starting merge…')
  console.log(`[merge]   client  : ${CLIENT_SRC}`)
  console.log(`[merge]   output  : ${OUTPUT_DIR}`)

  fs.mkdirSync(OUTPUT_DIR, { recursive: true })

  const SKIP_ALWAYS = new Set(['.git', '.gitignore', '.gitattributes'])

  const clientFiles = copyDir(CLIENT_SRC, OUTPUT_DIR, SKIP_ALWAYS)
  console.log(`[merge]   ${clientFiles} file(s) from client`)

  console.log(`[merge] Files merged: ${clientFiles} total in ${Date.now() - startMs}ms`)

  // ── Build distributable zip ──────────────────────────────────────────────────
  console.log('[merge] Building zip…')
  const zipStart = Date.now()
  const zipSize  = await buildZip(OUTPUT_DIR, ZIP_PATH)
  console.log(`[merge] Zip built: ${(zipSize / 1024 / 1024).toFixed(1)} MB in ${Date.now() - zipStart}ms`)

  // ── Write version file ───────────────────────────────────────────────────────
  // Version is content-based: client git commit hash.
  //   • Same commit → same version → launcher skips re-download after restart
  //   • New commit  → new version  → launcher re-downloads automatically
  const version = clientGitHash()
  fs.mkdirSync(path.dirname(VERSION_FILE), { recursive: true })
  fs.writeFileSync(VERSION_FILE, JSON.stringify({
    version,
    builtAt:   new Date().toISOString(),
    fileCount: clientFiles,
    zipSize,
  }, null, 2) + '\n')
  console.log(`[merge] Version: ${version}`)

  return { clientFiles, total: clientFiles, zipSize }
}

// ── CLI entry ─────────────────────────────────────────────────────────────────

if (require.main === module) {
  mergeSourcesIntoRoot().catch(err => {
    console.error('[merge] Fatal:', err.message)
    process.exit(1)
  })
}

module.exports = { mergeSourcesIntoRoot }
