'use strict'

/**
 * Mod Organizer 2 integration — portable install fully managed by the launcher.
 *
 *   %LOCALAPPDATA%\SkyRP\MO2\
 *     ModOrganizer.exe          downloaded from the official MO2 release
 *     ModOrganizer.ini          portable instance config (written by us)
 *     nxmhandler.ini            nxm:// → this MO2 instance
 *     downloads\                Nexus "Mod Manager Download" archives land here
 *     mods\<Mod Name>\          installed mods (extracted by us or by MO2)
 *     profiles\SkyRP\           the single launcher-managed profile
 *
 * Responsibilities:
 *  - Download + extract the pinned MO2 release (7z, via 7zip-bin).
 *  - Create/refresh the portable instance and SkyRP profile.
 *  - Register the nxm:// protocol so Nexus downloads land in our instance.
 *  - Auto-extract finished downloads into mods\ and enable them in the profile.
 *  - Launch the game through MO2's VFS (SKSE executable entry).
 */

const path = require('path')
const fs   = require('fs')
const os   = require('os')
const https = require('https')
const { spawn, execSync, execFileSync } = require('child_process')

const MO2_VERSION = '2.5.2'
const MO2_URL     = `https://github.com/ModOrganizer2/modorganizer/releases/download/v${MO2_VERSION}/Mod.Organizer-${MO2_VERSION}.7z`
const PROFILE     = 'SkyRP'

// ── Logger ────────────────────────────────────────────────────────────────────
let _log = (...args) => console.log('[mo2]', ...args)
function setLogger(fn) { _log = (...args) => fn('[mo2]', ...args) }

// ── Paths ─────────────────────────────────────────────────────────────────────

function getRoot() {
  const local = process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local')
  return path.join(local, 'SkyRP', 'MO2')
}

const getExe          = () => path.join(getRoot(), 'ModOrganizer.exe')
const getDownloadsDir = () => path.join(getRoot(), 'downloads')
const getModsDir      = () => path.join(getRoot(), 'mods')
const getProfileDir   = () => path.join(getRoot(), 'profiles', PROFILE)

function isInstalled() {
  return fs.existsSync(getExe())
}

// 7za ships inside the 7zip-bin npm package
function get7za() {
  const sevenBin = require('7zip-bin')
  return sevenBin.path7za.replace('app.asar', 'app.asar.unpacked')
}

// ── Download / install MO2 ────────────────────────────────────────────────────

/** Download url to dest, following redirects (GitHub releases redirect to a CDN). */
function downloadFile(url, dest, onProgress, redirectsLeft = 5) {
  return new Promise((resolve, reject) => {
    const req = https.get(url, res => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        res.resume()
        if (redirectsLeft <= 0) return reject(new Error('Too many redirects'))
        return resolve(downloadFile(res.headers.location, dest, onProgress, redirectsLeft - 1))
      }
      if (res.statusCode !== 200) {
        res.resume()
        return reject(new Error(`HTTP ${res.statusCode} downloading ${url}`))
      }

      const total = parseInt(res.headers['content-length'] || '0', 10)
      let received = 0
      const file = fs.createWriteStream(dest)
      res.on('data', chunk => {
        received += chunk.length
        if (onProgress) onProgress(received, total)
      })
      res.pipe(file)
      file.on('finish', () => file.close(resolve))
      file.on('error', err => { try { fs.unlinkSync(dest) } catch {} reject(err) })
      res.on('error',  err => { try { fs.unlinkSync(dest) } catch {} reject(err) })
    })
    req.on('error', reject)
    req.setTimeout(120_000, () => { req.destroy(); reject(new Error('Download timed out')) })
  })
}

/** Extract a .7z/.zip archive with the bundled 7za. */
function extractArchive(archivePath, destDir) {
  fs.mkdirSync(destDir, { recursive: true })
  execFileSync(get7za(), ['x', '-y', `-o${destDir}`, archivePath], {
    stdio: 'ignore',
    timeout: 10 * 60 * 1000,
  })
}

/** List entry paths inside an archive (used for FOMOD detection). */
function listArchive(archivePath) {
  try {
    const out = execFileSync(get7za(), ['l', '-ba', '-slt', archivePath], {
      encoding: 'utf8',
      timeout: 60_000,
      maxBuffer: 32 * 1024 * 1024,
    })
    return out.split(/\r?\n/)
      .filter(l => l.startsWith('Path = '))
      .map(l => l.slice('Path = '.length))
  } catch {
    return null
  }
}

/**
 * Download and unpack MO2 itself. Resolves immediately if already installed.
 * onProgress(message) receives human-readable status lines.
 */
async function ensureInstalled(onProgress) {
  if (isInstalled()) return

  const root    = getRoot()
  const archive = path.join(os.tmpdir(), `mo2-${MO2_VERSION}.7z`)

  _log(`installing MO2 ${MO2_VERSION} to ${root}`)
  if (onProgress) onProgress('Downloading Mod Organizer 2…')

  await downloadFile(MO2_URL, archive, (received, total) => {
    if (onProgress && total > 0) {
      const mb = n => (n / 1024 / 1024).toFixed(1)
      onProgress(`Downloading Mod Organizer 2… ${mb(received)} / ${mb(total)} MB`)
    }
  })

  if (onProgress) onProgress('Extracting Mod Organizer 2…')
  extractArchive(archive, root)
  try { fs.unlinkSync(archive) } catch {}

  if (!isInstalled()) {
    throw new Error('MO2 extraction finished but ModOrganizer.exe was not found.')
  }
  _log('MO2 installed')
}

// ── Portable instance / profile ───────────────────────────────────────────────

// Forward slashes everywhere: valid for Windows APIs and avoids INI escaping.
const fwd = p => p.replace(/\\/g, '/')

/**
 * Pick a dark stylesheet bundled with MO2 (preference order, then any *.qss
 * with "dark" in the name). Returns '' if none found.
 */
function pickDarkStyle() {
  const dir = path.join(getRoot(), 'stylesheets')
  const preferred = ['Paper Dark.qss', 'paper-dark.qss', 'VS15.qss', 'dark.qss', '1809.qss']
  try {
    const files = fs.readdirSync(dir)
    for (const name of preferred) {
      if (files.includes(name)) return name
    }
    const anyDark = files.find(f => /dark/i.test(f) && f.toLowerCase().endsWith('.qss'))
    if (anyDark) return anyDark
  } catch { /* stylesheets dir missing */ }
  return ''
}

/**
 * Create or refresh the portable instance config and the SkyRP profile.
 * Safe to call repeatedly; user data (mods, downloads) is never touched.
 *
 * @param {string}   skyrimPath
 * @param {string[]} [loadOrder]  Server esp/esm order for the profile's plugins.txt
 */
function ensureInstance(skyrimPath, loadOrder) {
  const root = getRoot()
  for (const dir of [getDownloadsDir(), getModsDir(), getProfileDir(), path.join(root, 'overwrite')]) {
    fs.mkdirSync(dir, { recursive: true })
  }

  // portable.txt is MO2's portable-instance marker. Without it MO2 ignores
  // the local ModOrganizer.ini and opens the user's registry-selected
  // (global) instance instead.
  fs.writeFileSync(path.join(root, 'portable.txt'), '')

  const style = pickDarkStyle()

  // ModOrganizer.ini — rewritten on every call so a moved Skyrim folder heals.
  const ini = [
    '[General]',
    'gameName=Skyrim Special Edition',
    `gamePath=@ByteArray(${fwd(skyrimPath)})`,
    `selected_profile=@ByteArray(${PROFILE})`,
    `version=${MO2_VERSION}`,
    'first_start=false',
    '',
    '[Settings]',
    'check_for_updates=false',
    ...(style ? [`style=${style}`] : []),
    '',
    '[customExecutables]',
    'size=1',
    '1\\title=SKSE',
    `1\\binary=${fwd(path.join(skyrimPath, 'skse64_loader.exe'))}`,
    `1\\workingDirectory=${fwd(skyrimPath)}`,
    '1\\arguments=',
    '1\\hide=false',
    '1\\toolbar=true',
    '1\\ownicon=true',
    '',
  ].join('\r\n')
  fs.writeFileSync(path.join(root, 'ModOrganizer.ini'), ini)

  // Profile files — only created when missing so MO2-side changes survive.
  const modlistPath = path.join(getProfileDir(), 'modlist.txt')
  if (!fs.existsSync(modlistPath)) {
    fs.writeFileSync(modlistPath, '# This file was automatically generated by Mod Organizer.\r\n')
  }

  const pluginsPath = path.join(getProfileDir(), 'plugins.txt')
  if (Array.isArray(loadOrder) && loadOrder.length > 0) {
    const vanilla = new Set(['skyrim.esm', 'update.esm', 'dawnguard.esm', 'hearthfires.esm', 'dragonborn.esm'])
    const lines = loadOrder
      .map(f => path.basename(f))
      .filter(f => !vanilla.has(f.toLowerCase()))
      .map(f => `*${f}`)
    fs.writeFileSync(pluginsPath,
      '# This file was automatically generated by Mod Organizer.\r\n' + lines.join('\r\n') + '\r\n')
  } else if (!fs.existsSync(pluginsPath)) {
    fs.writeFileSync(pluginsPath, '# This file was automatically generated by Mod Organizer.\r\n')
  }
}

/**
 * Point the nxm:// protocol at our portable instance so Nexus
 * "Mod Manager Download" buttons feed MO2's downloads folder.
 */
function registerNxmHandler() {
  const root       = getRoot()
  const nxmHandler = path.join(root, 'nxmhandler.exe')

  fs.writeFileSync(path.join(root, 'nxmhandler.ini'), [
    '[handlers]',
    'size=1',
    '1\\games=skyrimse',
    `1\\executable=${fwd(getExe())}`,
    '1\\arguments=',
    '',
  ].join('\r\n'))

  if (process.platform !== 'win32') return
  try {
    const run = cmd => execSync(cmd, { timeout: 5000, stdio: 'ignore' })
    run(`reg add "HKCU\\Software\\Classes\\nxm" /ve /d "URL:NXM Protocol" /f`)
    run(`reg add "HKCU\\Software\\Classes\\nxm" /v "URL Protocol" /d "" /f`)
    run(`reg add "HKCU\\Software\\Classes\\nxm\\shell\\open\\command" /ve /d "\\"${nxmHandler}\\" \\"%1\\"" /f`)
    _log('nxm:// handler registered')
  } catch (err) {
    _log('nxm handler registration failed:', err.message)
  }
}

// ── Mod management ────────────────────────────────────────────────────────────

/** Find an installed mod folder by Nexus mod ID (meta.ini scan). Returns folder name or null. */
function findModByNexusId(nexusId) {
  const modsDir = getModsDir()
  if (!fs.existsSync(modsDir)) return null

  let dirs
  try { dirs = fs.readdirSync(modsDir, { withFileTypes: true }).filter(e => e.isDirectory()) }
  catch { return null }

  for (const entry of dirs) {
    try {
      const meta  = fs.readFileSync(path.join(modsDir, entry.name, 'meta.ini'), 'utf8')
      const match = meta.match(/^modid\s*=\s*(\d+)/im)
      if (match && parseInt(match[1], 10) === nexusId) return entry.name
    } catch { /* no meta.ini — skip */ }
  }
  return null
}

/** Find a finished/ongoing download archive for a Nexus mod ID. */
function findDownloadByNexusId(nexusId) {
  const dlDir = getDownloadsDir()
  if (!fs.existsSync(dlDir)) return null

  const needle = `-${nexusId}-`
  try {
    const files = fs.readdirSync(dlDir, { withFileTypes: true })
      .filter(e => !e.isDirectory())
      .filter(e => !e.name.endsWith('.unfinished') && !e.name.endsWith('.meta'))
    return files.find(f => f.name.includes(needle))?.name ?? null
  } catch {
    return null
  }
}

/**
 * Extract a downloaded archive into mods\<name>\ and write meta.ini.
 * FOMOD installers and .rar archives are left for the user to install
 * through MO2 itself (returned as { needsSetup: true }).
 */
function installModFromDownload(nexusId, modName) {
  const archiveName = findDownloadByNexusId(nexusId)
  if (!archiveName) return { folder: null, needsSetup: false }
  return installModFromArchive(archiveName, nexusId, modName)
}

/**
 * Same as installModFromDownload but with an explicitly named archive in
 * the downloads folder (used by the premium Nexus direct-download path).
 */
function installModFromArchive(archiveName, nexusId, modName) {
  const archivePath = path.join(getDownloadsDir(), archiveName)
  const folderName  = (modName || archiveName.replace(/(\.\d+)?\.[^.]+$/, '')).replace(/[<>:"/\\|?*]/g, '')
  const modDir      = path.join(getModsDir(), folderName)

  if (fs.existsSync(modDir)) return { folder: folderName, needsSetup: false }

  const entries = listArchive(archivePath)
  if (entries === null) {
    // 7za couldn't read it — likely still being written; retry next tick
    return { folder: null, needsSetup: false }
  }
  if (entries.some(e => /^fomod[\\/]/i.test(e))) {
    _log(`${archiveName}: FOMOD installer — needs manual install in MO2`)
    return { folder: null, needsSetup: true }
  }

  try {
    extractArchive(archivePath, modDir)
  } catch (err) {
    _log(`${archiveName}: extraction failed (${err.message})`)
    try { fs.rmSync(modDir, { recursive: true, force: true }) } catch {}
    return { folder: null, needsSetup: false }
  }

  fs.writeFileSync(path.join(modDir, 'meta.ini'), [
    '[General]',
    'gameName=SkyrimSE',
    `modid=${nexusId}`,
    `name=${folderName}`,
    `installationFile=${archiveName}`,
    'repository=Nexus',
    '',
  ].join('\r\n'))

  _log(`installed ${folderName} from ${archiveName}`)
  return { folder: folderName, needsSetup: false }
}

/** Enable a mod in the SkyRP profile (idempotent). */
function enableMod(folderName) {
  const modlistPath = path.join(getProfileDir(), 'modlist.txt')
  let lines = []
  try { lines = fs.readFileSync(modlistPath, 'utf8').split(/\r?\n/) } catch {}

  if (lines.includes(`+${folderName}`)) return
  lines = lines.filter(l => l !== `-${folderName}`)

  // First line of modlist.txt is the comment header; new mods go right under
  // it (top of the list = highest priority in MO2).
  const header = lines.length > 0 && lines[0].startsWith('#') ? [lines.shift()] : []
  fs.writeFileSync(modlistPath, [...header, `+${folderName}`, ...lines].join('\r\n'))
}

/**
 * Poll until every mod in missingMods is installed in mods\.
 * Mirrors the old Vortex wait loop: waiting → downloaded → installed,
 * with FOMOD/rar mods surfaced as "install through MO2".
 */
function waitForMods(missingMods, onProgress, signal, intervalMs = 4000, timeoutMs = 300_000) {
  const deadline = Date.now() + timeoutMs

  return new Promise((resolve, reject) => {
    function tick() {
      if (signal?.aborted) return reject(new Error('Cancelled'))
      if (Date.now() > deadline) {
        return reject(new Error(`Timed out waiting for ${missingMods.length} mod(s) to install.`))
      }

      const installed  = []
      const needsSetup = []
      const waiting    = []

      for (const mod of missingMods) {
        const existing = findModByNexusId(mod.nexusId)
        if (existing) {
          enableMod(existing)
          installed.push(mod)
          continue
        }
        const result = installModFromDownload(mod.nexusId, mod.name)
        if (result.folder) {
          enableMod(result.folder)
          installed.push(mod)
        } else if (result.needsSetup) {
          needsSetup.push(mod)
        } else {
          waiting.push(mod)
        }
      }

      if (onProgress) {
        let message
        if (needsSetup.length > 0) {
          message = `Install through MO2 (installer/rar): ${needsSetup.map(m => m.name).join(', ')}`
        } else if (waiting.length > 0) {
          message = `Waiting for download: ${waiting.map(m => m.name).join(', ')}`
        } else {
          message = 'All mods installed'
        }
        onProgress(installed.length, missingMods.length, message)
      }

      if (installed.length === missingMods.length) return resolve()
      setTimeout(tick, intervalMs)
    }

    setTimeout(tick, intervalMs)
  })
}

// ── Launch ────────────────────────────────────────────────────────────────────

/** Launch the game through MO2's VFS using the SKSE executable entry. */
function launchGame() {
  if (!isInstalled()) throw new Error('MO2 is not installed — run setup in Settings first.')
  spawn(getExe(), ['-p', PROFILE, 'moshortcut://:SKSE'], {
    detached: true,
    stdio: 'ignore',
    cwd: getRoot(),
  }).unref()
}

/** Open the MO2 UI itself (for manual mod management / FOMOD installs). */
function openUI() {
  if (!isInstalled()) throw new Error('MO2 is not installed.')
  spawn(getExe(), ['-p', PROFILE], { detached: true, stdio: 'ignore', cwd: getRoot() }).unref()
}

// ── Status ────────────────────────────────────────────────────────────────────

function getStatus() {
  let modCount = 0
  try { modCount = fs.readdirSync(getModsDir(), { withFileTypes: true }).filter(e => e.isDirectory()).length }
  catch {}
  return {
    installed: isInstalled(),
    version:   MO2_VERSION,
    root:      getRoot(),
    modCount,
  }
}

module.exports = {
  setLogger,
  PROFILE,
  getRoot,
  getDownloadsDir,
  getModsDir,
  getProfileDir,
  isInstalled,
  ensureInstalled,
  ensureInstance,
  registerNxmHandler,
  findModByNexusId,
  findDownloadByNexusId,
  installModFromDownload,
  installModFromArchive,
  enableMod,
  waitForMods,
  launchGame,
  openUI,
  getStatus,
}
