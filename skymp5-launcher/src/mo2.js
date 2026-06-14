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

let _rootProvider = null
function setRootProvider(fn) { _rootProvider = fn }

function getRoot() {
  const custom = _rootProvider ? _rootProvider() : null
  if (custom) return custom
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

// Detect the Skyrim SE store edition
function detectEdition(gameDir) {
  try {
    const names = fs.readdirSync(gameDir)
    if (names.includes('Galaxy64.dll') || names.some(f => /^goggame-.*\.(info|dll|hashdb)$/i.test(f))) return 'GOG'
    if (names.includes('EOSSDK-Win64-Shipping.dll')) return 'Epic Games'
    if (names.includes('steam_api64.dll')) return 'Steam'
  } catch { /* unreadable */ }
  return 'Steam'
}

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
    `gameEdition=${detectEdition(skyrimPath)}`,
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
 * Install the downloaded archive for a Nexus mod id, if one is present.
 * Returns the same shape as installModFromArchive: {folder} | {error} | {}.
 */
function installModFromDownload(nexusId, modName, exclude) {
  const archiveName = findDownloadByNexusId(nexusId)
  if (!archiveName) return {}
  return installModFromArchive(archiveName, nexusId, modName, exclude)
}

/**
 * Install a single named archive (URL mods and the free-account path).
 * Skips if already installed
 *
 * Returns { folder } | { error } | {}.
 */
function installModFromArchive(archiveName, nexusId, modName, exclude) {
  const folderName  = (modName || archiveName.replace(/(\.\d+)?\.[^.]+$/, '')).replace(/[<>:"/\\|?*]/g, '')
  const archivePath = path.join(getDownloadsDir(), archiveName)
  if (fs.existsSync(path.join(getModsDir(), folderName))) return { folder: folderName }
  if (!fs.existsSync(archivePath) || listArchive(archivePath) === null) return {}
  return installMod(folderName, [archiveName], { nexusId, exclude })
}

/** Remove author-specified relative paths from a staged mod folder. */
function applyExclude(modDir, exclude) {
  if (!Array.isArray(exclude)) return
  for (const rel of exclude) {
    const target = path.join(modDir, String(rel).replace(/[\\/]+/g, path.sep))
    try {
      if (fs.existsSync(lp(target))) { fs.rmSync(lp(target), { recursive: true, force: true }); _log(`  excluded ${rel}`) }
    } catch {}
  }
}

// ── Data-root detection ───────────────────────────────────────────────────────

const DATA_DIR_NAMES = new Set([
  'textures', 'meshes', 'scripts', 'interface', 'skse', 'sound', 'music',
  'strings', 'seq', 'grass', 'lodsettings', 'shadersfx', 'video', 'fonts',
  'platform', 'mcm', 'dialogueviews', 'calientetools', 'source', 'nemesis_engine',
])
const PLUGIN_FILE_RE = /\.(esp|esm|esl|bsa|ba2)$/i

function looksLikeDataRoot(dir) {
  let entries
  try { entries = fs.readdirSync(dir, { withFileTypes: true }) } catch { return false }
  return entries.some(e =>
    (!e.isDirectory() && PLUGIN_FILE_RE.test(e.name)) ||
    (e.isDirectory() && DATA_DIR_NAMES.has(e.name.toLowerCase())))
}

// ── Merge reconstructor (FOMOD-proof data-root union) ─────────────────────────

// Which Skyrim runtime we target, for resolving option-folder DLL conflicts.
let _gameTier = 'ae'  // SkyRP runs Anniversary Edition; refined by setGameDataDir
const TIER = {
  ae: { match: /(\bae\b|anniversary|1[._]6|skse64_?1_?6)/i, other: /(\bse\b|1[._]5|skse64_?1_?5|legendary)/i },
  se: { match: /(\bse\b|1[._]5|skse64_?1_?5|legendary)/i, other: /(\bae\b|anniversary|1[._]6|skse64_?1_?6)/i },
}

/** Recursively copy src into dst, overwriting. Returns files copied. */
// Windows caps fs paths at MAX_PATH (260) unless prefixed with \\?\. Big mods
// like JK's have mesh paths past that, so plain Node copies fail where MO2
// itself (which opts into long paths) succeeds. Prefix every fs boundary.
function lp(p) {
  if (process.platform !== 'win32') return p
  const abs = path.resolve(p)
  return abs.startsWith('\\\\?\\') ? abs : '\\\\?\\' + abs
}

function mergeDir(src, dst) {
  let n = 0
  let entries
  try { entries = fs.readdirSync(lp(src), { withFileTypes: true }) } catch { return 0 }
  fs.mkdirSync(lp(dst), { recursive: true })
  for (const e of entries) {
    const s = path.join(src, e.name)
    const d = path.join(dst, e.name)
    if (e.isDirectory()) n += mergeDir(s, d)
    else { fs.copyFileSync(lp(s), lp(d)); n++ }   // overwrite — later sources win
  }
  return n
}

/**
 * Merge a source root into modDir, treating known Data sub-dirs and loose
 * files as content (copied at their level) and everything else as an option
 * folder whose contents are unioned in. Tier-matching option folders are
 * applied LAST so the correct game-version files win conflicts.
 */
function mergeRoot(srcRoot, modDir, depth) {
  if (depth > 8) return 0
  let entries
  try { entries = fs.readdirSync(lp(srcRoot), { withFileTypes: true }) } catch { return 0 }

  fs.mkdirSync(lp(modDir), { recursive: true })
  let n = 0
  const optionDirs = []

  for (const e of entries) {
    if (e.name.toLowerCase() === 'fomod') continue       // FOMOD control folder
    const abs = path.join(srcRoot, e.name)
    if (!e.isDirectory()) {
      const d = path.join(modDir, e.name)
      fs.mkdirSync(lp(path.dirname(d)), { recursive: true })
      fs.copyFileSync(lp(abs), lp(d)); n++                // loose root file
    } else if (DATA_DIR_NAMES.has(e.name.toLowerCase())) {
      n += mergeDir(abs, path.join(modDir, e.name))       // real Data sub-dir
    } else {
      optionDirs.push({ abs, name: e.name })              // option / wrapper folder
    }
  }

  // Apply option folders: wrong-tier first, neutral next, matching-tier last.
  const t = TIER[_gameTier] || TIER.ae
  const rank = o => t.match.test(o.name) ? 2 : (t.other.test(o.name) ? 0 : 1)
  optionDirs.sort((a, b) => rank(a) - rank(b))
  for (const o of optionDirs) n += mergeRoot(o.abs, modDir, depth + 1)

  return n
}

/** Refine the game tier (AE vs SE) from the installed SKSE runtime so merge
 *  conflicts pick the right game-version files. `dir` is the game Data folder. */
function setGameDataDir(dir) {
  try {
    const rootNames = fs.readdirSync(path.dirname(dir)).join(' ').toLowerCase()
    if (/skse64_1_5/.test(rootNames)) _gameTier = 'se'
    else if (/skse64_1_6/.test(rootNames)) _gameTier = 'ae'
  } catch { /* skse not installed yet — keep AE default */ }
}

/** Find an installed mod folder by its meta.ini display name. */
function findModByName(modName) {
  const wanted = String(modName).toLowerCase()
  try {
    for (const e of fs.readdirSync(getModsDir(), { withFileTypes: true })) {
      if (!e.isDirectory()) continue
      if (e.name.toLowerCase() === wanted) return e.name
      try {
        const meta  = fs.readFileSync(path.join(getModsDir(), e.name, 'meta.ini'), 'utf8')
        const match = meta.match(/^name\s*=\s*(.+)$/im)
        if (match && match[1].trim().toLowerCase() === wanted) return e.name
      } catch {}
    }
  } catch {}
  return null
}

// ── Root-directory installs (SKSE & co.) ─────────────────────────────────────

/**
 * Install an archive whose binaries belong in the GAME ROOT (e.g. SKSE:
 * skse64_loader.exe + DLLs next to SkyrimSE.exe, scripts under Data/).
 * Root-level exe/dll files are copied into gameDir; a Data folder, if
 * present, becomes a regular MO2 mod so the VFS serves its scripts.
 *
 * @returns {{ folder: string|null }}  the created scripts-mod folder, if any
 */
function installRootArchive(archiveName, gameDir, modName) {
  const archivePath = path.join(getDownloadsDir(), archiveName)
  const stagingDir  = path.join(getRoot(), '.staging', modName.replace(/[<>:"/\\|?*]/g, ''))
  try { fs.rmSync(stagingDir, { recursive: true, force: true }) } catch {}

  if (listArchive(archivePath) === null) {
    throw new Error(`${modName}: the downloaded file is not a valid archive — the URL may point to a web page, not a direct download.`)
  }
  extractArchive(archivePath, stagingDir)

  // Descend through a single wrapper folder (skse64_2_02_06/…)
  let root = stagingDir
  for (let i = 0; i < 3; i++) {
    const entries = fs.readdirSync(root, { withFileTypes: true })
    const dirs    = entries.filter(e => e.isDirectory())
    const files   = entries.filter(e => !e.isDirectory())
    if (files.some(f => /\.(exe|dll)$/i.test(f.name))) break
    if (dirs.length === 1 && files.length === 0) { root = path.join(root, dirs[0].name); continue }
    break
  }

  let copied = 0
  for (const e of fs.readdirSync(root, { withFileTypes: true })) {
    if (!e.isDirectory() && /\.(exe|dll)$/i.test(e.name)) {
      fs.copyFileSync(path.join(root, e.name), path.join(gameDir, e.name))
      copied++
    }
  }
  if (copied === 0) {
    try { fs.rmSync(stagingDir, { recursive: true, force: true }) } catch {}
    throw new Error(`${modName}: no .exe/.dll found in the archive — is this a valid SKSE-style download?`)
  }
  _log(`${modName}: copied ${copied} root file(s) into ${gameDir}`)

  let folder = null
  const dataDir = fs.readdirSync(root, { withFileTypes: true })
    .find(e => e.isDirectory() && e.name.toLowerCase() === 'data')
  if (dataDir) {
    folder = `${modName} (data)`.replace(/[<>:"/\\|?*]/g, '')
    const modDir = path.join(getModsDir(), folder)
    fs.mkdirSync(modDir, { recursive: true })
    const src = path.join(root, dataDir.name)
    for (const entry of fs.readdirSync(src)) {
      fs.renameSync(path.join(src, entry), path.join(modDir, entry))
    }
    fs.writeFileSync(path.join(modDir, 'meta.ini'), [
      '[General]', 'gameName=SkyrimSE', 'modid=0', `name=${folder}`,
      `installationFile=${archiveName}`, 'repository=', '',
    ].join('\r\n'))
    _log(`${modName}: Data payload installed as mod "${folder}"`)
  }

  try { fs.rmSync(stagingDir, { recursive: true, force: true }) } catch {}
  return { folder }
}

/** Download any URL into the MO2 downloads folder. Returns the archive name. */
async function downloadToDownloads(url, fileName, onProgress) {
  const dest = path.join(getDownloadsDir(), fileName)
  if (fs.existsSync(dest)) return fileName
  fs.mkdirSync(getDownloadsDir(), { recursive: true })
  const temp = dest + '.unfinished'
  await downloadFile(url, temp, onProgress)
  fs.renameSync(temp, dest)
  return fileName
}

/** Stable signature of an exclude list (order-independent). */
function excludeSig(exclude) {
  return JSON.stringify((Array.isArray(exclude) ? exclude : []).map(String).sort())
}

/**
 * Read what's installed for a Nexus mod: { folder, sig } or null.
 * `sig` is the skyrpSig written by installMod (captures file ids + excludes),
 * so the installer can tell when a mod needs updating vs. is up to date.
 */
function getInstalledInfo(nexusId) {
  const folder = findModByNexusId(nexusId)
  if (!folder) return null
  let sig = null
  try {
    const meta = fs.readFileSync(path.join(getModsDir(), folder, 'meta.ini'), 'utf8')
    const m = meta.match(/^skyrpSig\s*=\s*(.*)$/im)
    sig = m ? m[1].trim() : null
  } catch {}
  return { folder, sig }
}

/** Write/replace skyrpSig in an installed mod's meta.ini (used by the free path). */
function stampSig(nexusId, sig) {
  const folder = findModByNexusId(nexusId)
  if (!folder) return
  try {
    const metaPath = path.join(getModsDir(), folder, 'meta.ini')
    let meta = fs.existsSync(metaPath) ? fs.readFileSync(metaPath, 'utf8') : '[General]\r\n'
    if (/^skyrpSig\s*=/im.test(meta)) meta = meta.replace(/^skyrpSig\s*=.*$/im, `skyrpSig=${sig}`)
    else meta = meta.replace(/\[General\]/i, `[General]\r\nskyrpSig=${sig}`)
    fs.writeFileSync(metaPath, meta)
  } catch {}
}

/**
 * (Re)install a mod from one or more archives, OVERWRITING any existing
 * install, then stamp meta.ini with `skyrpSig` so future runs can detect
 * changes. Archives are merged in order (later overlays earlier), so the
 * main file comes first and optional files after.
 *
 * @returns { folder } | { error }
 */
function installMod(modName, archiveNames, { nexusId, exclude, sig } = {}) {
  const folderName = String(modName).replace(/[<>:"/\\|?*]/g, '')
  const modDir     = path.join(getModsDir(), folderName)
  // Build in a SHORT temp dir and only swap into place on success — a failed
  // (re)install must never destroy a working existing/manual install, and the
  // short build path keeps deep-mesh mods under MAX_PATH while building.
  const id         = String(_buildCounter++)
  const buildDir   = path.join(getRoot(), '.b', id)
  const stagingDir = path.join(getRoot(), '.s', id)

  try { fs.rmSync(lp(buildDir), { recursive: true, force: true }) } catch {}
  try {
    fs.mkdirSync(lp(buildDir), { recursive: true })
    let total = 0
    for (const archiveName of archiveNames) {
      const archivePath = path.join(getDownloadsDir(), archiveName)
      if (!fs.existsSync(archivePath) || listArchive(archivePath) === null) {
        throw new Error(`archive not ready: ${archiveName}`)
      }
      try { fs.rmSync(lp(stagingDir), { recursive: true, force: true }) } catch {}
      extractArchive(archivePath, stagingDir)
      const n = mergeRoot(stagingDir, buildDir, 0)
      _log(`  ${archiveName}: merged ${n} file(s)`)
      total += n
    }
    if (total === 0) throw new Error('archives contained no installable files (check the file/version pin)')
    if (!looksLikeDataRoot(buildDir)) _log(`  ${folderName}: no plugins/data dirs detected — VERIFY in MO2`)

    applyExclude(buildDir, exclude)

    fs.writeFileSync(lp(path.join(buildDir, 'meta.ini')), [
      '[General]', 'gameName=SkyrimSE', `modid=${nexusId ?? 0}`, `name=${folderName}`,
      'repository=Nexus', `skyrpSig=${sig || ''}`, '',
    ].join('\r\n'))

    // Swap: remove the old install only now, then move the new one in.
    try { fs.rmSync(lp(modDir), { recursive: true, force: true }) } catch {}
    fs.renameSync(lp(buildDir), lp(modDir))

    _log(`installed ${folderName} (${total} file(s) from ${archiveNames.length} archive(s))`)
    return { folder: folderName }
  } catch (err) {
    try { fs.rmSync(lp(buildDir), { recursive: true, force: true }) } catch {}
    return { error: err.message }   // existing modDir left intact
  } finally {
    try { fs.rmSync(lp(stagingDir), { recursive: true, force: true }) } catch {}
  }
}
let _buildCounter = 1

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
 * Poll until every mod in missingMods is installed in mods\ (used by the
 * free-account path, where downloads arrive asynchronously via the nxm
 * handler). A mod only counts as "waiting" while its archive isn't on disk;
 * an archive that is present but fails to install rejects immediately with
 * the real error — it is never retried forever as "waiting for download".
 */
function waitForMods(missingMods, onProgress, signal, intervalMs = 4000, timeoutMs = 900_000) {
  const deadline = Date.now() + timeoutMs

  return new Promise((resolve, reject) => {
    function tick() {
      if (signal?.aborted) return reject(new Error('Cancelled'))

      const installed = []
      const waiting   = []

      for (const mod of missingMods) {
        const existing = findModByNexusId(mod.nexusId)
        if (existing) { enableMod(existing); installed.push(mod); continue }

        const res = installModFromDownload(mod.nexusId, mod.name, mod.exclude)
        if (res.folder)      { enableMod(res.folder); installed.push(mod) }
        else if (res.error)  return reject(new Error(`${mod.name}: ${res.error}`))
        else                 waiting.push(mod)
      }

      if (onProgress) {
        const message = waiting.length > 0
          ? `Waiting for download: ${waiting.map(m => m.name).join(', ')}`
          : 'All mods installed'
        onProgress(installed.length, missingMods.length, message)
      }

      if (installed.length === missingMods.length) return resolve()
      if (Date.now() > deadline) {
        return reject(new Error(`Timed out waiting to download: ${waiting.map(m => m.name).join(', ')}`))
      }
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
  setRootProvider,
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
  installMod,
  getInstalledInfo,
  excludeSig,
  stampSig,
  installRootArchive,
  downloadToDownloads,
  findModByName,
  setGameDataDir,
  enableMod,
  waitForMods,
  launchGame,
  openUI,
  getStatus,
}
