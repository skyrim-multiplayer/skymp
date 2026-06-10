'use strict'

/**
 * Vortex Mod Manager integration.
 *
 * Responsibilities:
 *  - Detect the Vortex installation and data directory.
 *  - Manage a dedicated "skyrp-client" mod in Vortex's staging area.
 *  - Keep the selected profile's modlist.txt up to date.
 *  - Deploy staged files to the Skyrim directory via hardlinks (or copy as fallback).
 */

const path         = require('path')
const fs           = require('fs')
const os           = require('os')
const { execSync } = require('child_process')

const GAME_ID = 'skyrimse'

// ── Logger ────────────────────────────────────────────────────────────────────
// Call setLogger(fn) from main.js to route vortex debug output through the
// app's log function (console + file).  Falls back to console.log.
let _log = (...args) => console.log('[vortex]', ...args)
function setLogger(fn) { _log = (...args) => fn('[vortex]', ...args) }

// ── Path helpers ──────────────────────────────────────────────────────────────

/** %APPDATA%\Vortex */
function getDataPath() {
  return path.join(
    process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming'),
    'Vortex'
  )
}

/**
 * Root of Vortex's mod staging directory for Skyrim SE.
 * Pass an explicit override (from user settings) to support non-default paths.
 * Falls back to %APPDATA%\Vortex\skyrimse\mods.
 *
 * @param {string} [override]
 */
function getStagingRoot(override) {
  if (override && override.trim()) return override.trim()
  return path.join(getDataPath(), GAME_ID, 'mods')
}

/** %APPDATA%\Vortex\downloads\skyrimse */
function getDownloadsRoot() {
  return path.join(getDataPath(), 'downloads', GAME_ID)
}

/** .../Vortex/skyrimse/profiles */
function getProfilesRoot() {
  return path.join(getDataPath(), GAME_ID, 'profiles')
}

/** .../Vortex/skyrimse/profiles/<profileId> */
function getProfileDir(profileId) {
  return path.join(getProfilesRoot(), profileId)
}

// ── Vortex detection ──────────────────────────────────────────────────────────

/**
 * Locate Vortex.exe using three strategies in order:
 *
 *  1. Windows registry — nxm:// protocol handler key written by the Vortex
 *     installer.  This is the most reliable source and works regardless of
 *     where the user chose to install Vortex.
 *  2. Uninstall registry entry — present even if the nxm handler was removed.
 *  3. Hardcoded path candidates — fallback for portable / non-standard installs.
 *
 * Returns the absolute path to Vortex.exe, or null if not found.
 */
function findVortexExe() {
  // 1. nxm:// protocol handler (HKCU) — written on first run / reinstall
  try {
    const out = execSync(
      'reg query "HKCU\\Software\\Classes\\nxm\\shell\\open\\command" /ve',
      { encoding: 'utf8', timeout: 3000, stdio: ['ignore', 'pipe', 'ignore'] }
    )
    const match = out.match(/"([^"]+\.exe)"/)
    if (match && fs.existsSync(match[1])) return match[1]
  } catch { /* registry key absent or reg.exe unavailable */ }

  // 2. Uninstall entry in HKLM (written by the NSIS installer)
  try {
    const out = execSync(
      'reg query "HKLM\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall" /s /f "Vortex" /t REG_SZ',
      { encoding: 'utf8', timeout: 5000, stdio: ['ignore', 'pipe', 'ignore'] }
    )
    const match = out.match(/InstallLocation\s+REG_SZ\s+(.+)/i)
    if (match) {
      const candidate = path.join(match[1].trim(), 'Vortex.exe')
      if (fs.existsSync(candidate)) return candidate
    }
  } catch { /* not in registry */ }

  // 3. Hardcoded fallbacks for non-standard / portable installs
  const local = process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local')
  const candidates = [
    path.join(local, 'Programs', 'black_tree_gaming', 'Vortex', 'Vortex.exe'),
    path.join(local, 'Programs', 'Vortex', 'Vortex.exe'),
    'C:\\Program Files\\Black Tree Gaming Ltd\\Vortex\\Vortex.exe',
    'C:\\Program Files\\Black Tree Gaming\\Vortex\\Vortex.exe',
    'C:\\Program Files (x86)\\Black Tree Gaming\\Vortex\\Vortex.exe',
  ]
  return candidates.find(p => fs.existsSync(p)) ?? null
}

// ── Profile listing ───────────────────────────────────────────────────────────

/**
 * List profiles from the Vortex filesystem (directory scan only).
 * Names come from a .skyrp marker file if previously tagged, otherwise
 * the raw profile ID is used.  For real Vortex names, call readProfilesFromState().
 *
 * @returns {{ id: string, name: string }[]}
 */
function listProfiles() {
  const root = getProfilesRoot()
  if (!fs.existsSync(root)) return []

  return fs.readdirSync(root, { withFileTypes: true })
    .filter(e => e.isDirectory())
    .map(e => {
      let name = e.name
      try {
        const marker = JSON.parse(
          fs.readFileSync(path.join(root, e.name, '.skyrp'), 'utf8')
        )
        if (marker.profileName) name = marker.profileName
      } catch { /* marker absent – use the raw ID */ }
      return { id: e.name, name }
    })
}

/**
 * If exactly one skyrimse profile exists on the filesystem, return it so the
 * caller can auto-select it without user interaction.
 * Returns { id, name } or null if 0 or 2+ profiles exist.
 */
function autoDetectProfile() {
  const profiles = listProfiles()
  return profiles.length === 1 ? profiles[0] : null
}

/**
 * Read profile names and the active profile for skyrimse directly from
 * Vortex's LevelDB state store (%APPDATA%\Vortex\state.v2).
 *
 * Vortex persists its Redux state to LevelDB using '###' as a path separator,
 * so profile entries look like:
 *   key:   persistent###profiles###<profileId>###name
 *   value: "Default"
 *
 * The active profile is stored at:
 *   key:   settings###profiles###activeProfileId###skyrimse
 *   value: "<profileId>"
 *
 * Falls back to the filesystem-only listing if:
 *  - classic-level is not installed
 *  - LevelDB is locked (Vortex is running)
 *  - Any other read error
 *
 * @returns {Promise<{ profiles: {id: string, name: string}[], activeProfileId: string|null }>}
 */
async function readProfilesFromState() {
  const dbPath = path.join(getDataPath(), 'state.v2')
  if (!fs.existsSync(dbPath)) return { profiles: listProfiles(), activeProfileId: null }

  let db
  try {
    const { ClassicLevel } = require('classic-level')
    db = new ClassicLevel(dbPath, { keyEncoding: 'utf8', valueEncoding: 'utf8' })

    // Accumulate profile fields by profile ID.
    // Keys: persistent###profiles###<profileId>###<field>
    const profileMap = new Map()

    for await (const [key, value] of db.iterator({
      gte: 'persistent###profiles###',
      lte: 'persistent###profiles###\xFF',
    })) {
      const parts = key.split('###')
      // Need at least: persistent, profiles, <profileId>, <field>
      if (parts.length < 4) continue

      const profileId = parts[2]
      const field     = parts[3]

      // Only collect the fields we need to avoid reading large modState blobs
      if (field !== 'id' && field !== 'name' && field !== 'gameId') continue

      if (!profileMap.has(profileId)) profileMap.set(profileId, { id: profileId })

      let parsed
      try { parsed = JSON.parse(value) } catch { parsed = value }
      profileMap.get(profileId)[field] = parsed
    }

    const profiles = [...profileMap.values()]
      .filter(p => p.gameId === GAME_ID && p.name)
      .map(p => ({ id: p.id, name: String(p.name) }))

    // Read the active profile for this game
    let activeProfileId = null
    try {
      const raw = await db.get(`settings###profiles###activeProfileId###${GAME_ID}`)
      try { activeProfileId = JSON.parse(raw) } catch { activeProfileId = raw }
    } catch { /* key absent — Vortex never activated a profile for this game */ }

    return {
      profiles: profiles.length > 0 ? profiles : listProfiles(),
      activeProfileId,
    }
  } catch {
    // LevelDB locked (Vortex is running) or classic-level not available
    return { profiles: listProfiles(), activeProfileId: null }
  } finally {
    await db?.close().catch(() => {})
  }
}

/**
 * Write a .skyrp marker to the profile directory so we can recall the
 * human-readable name on subsequent launches (Vortex stores names in LevelDB).
 */
function tagProfile(profileId, profileName) {
  const dir = getProfileDir(profileId)
  if (!fs.existsSync(dir)) fs.mkdirSync(dir, { recursive: true })
  fs.writeFileSync(
    path.join(dir, '.skyrp'),
    JSON.stringify({ profileName, tagged: new Date().toISOString() })
  )
}


/**
 * Scan the Vortex staging directory for a mod matching the given Nexus mod ID.
 * Returns the staging folder name (= the mod ID used in modlist.txt) or null.
 *
 * Detection order (first match wins):
 *  1. meta.ini modid field  — authoritative; handles any folder naming scheme
 *  2. Folder name prefix    — Vortex often names folders "{nexusId}-{version}-…"
 *
 * Prefer findEnabledModByNexusId() when a profileId is available — it is faster
 * because it only checks the mods Vortex has already enabled.
 *
 * @param {number} nexusId
 * @param {string} [stagingOverride]  User-configured staging path (optional)
 * @returns {string|null}
 */
function findModByNexusId(nexusId, stagingOverride) {
  const stagingRoot = getStagingRoot(stagingOverride)
  _log(`findModByNexusId(${nexusId}) stagingRoot=${stagingRoot}`)

  if (!fs.existsSync(stagingRoot)) {
    _log(`  staging root does not exist`)
    return null
  }

  const idStr  = String(nexusId)
  let   dirs
  try { dirs = fs.readdirSync(stagingRoot, { withFileTypes: true }).filter(e => e.isDirectory()) }
  catch (err) { _log(`  readdirSync failed: ${err.message}`); return null }

  _log(`  scanning ${dirs.length} staging folder(s): ${dirs.map(d => d.name).join(', ')}`)

  for (const entry of dirs) {
    // ── 1. meta.ini modid ────────────────────────────────────────────────────
    const metaPath = path.join(stagingRoot, entry.name, 'meta.ini')
    if (fs.existsSync(metaPath)) {
      try {
        const content = fs.readFileSync(metaPath, 'utf8')
        const match   = content.match(/^modid\s*=\s*(\d+)/im)
        _log(`  ${entry.name}/meta.ini → modid=${match ? match[1] : '(not found)'}`)
        if (match && parseInt(match[1], 10) === nexusId) {
          _log(`  MATCH via meta.ini: ${entry.name}`)
          return entry.name
        }
      } catch (err) { _log(`  ${entry.name}/meta.ini read error: ${err.message}`) }
    } else {
      _log(`  ${entry.name} has no meta.ini`)
    }

    // ── 2. Folder name prefix ─────────────────────────────────────────────────
    if (entry.name.startsWith(idStr + '-') || entry.name === idStr) {
      _log(`  MATCH via folder name prefix: ${entry.name}`)
      return entry.name
    }
  }

  _log(`  no match found for nexusId=${nexusId}`)
  return null
}

/**
 * Check whether a Nexus mod is installed AND enabled in a specific Vortex profile.
 *
 * Uses the profile's modlist.txt as the source of truth — a `+` entry there means
 * Vortex considers the mod installed and enabled.  Only the enabled staging folders
 * are inspected (via meta.ini / name prefix), making this faster and more accurate
 * than a full staging scan.
 *
 * Falls back to findModByNexusId() (full scan) when no profileId is given, or when
 * the modlist file cannot be read.
 *
 * @param {number} nexusId
 * @param {string} profileId
 * @param {string} [stagingOverride]
 * @returns {string|null}  Staging folder name if found, otherwise null
 */
function findEnabledModByNexusId(nexusId, profileId, stagingOverride) {
  _log(`findEnabledModByNexusId(${nexusId}) profileId=${profileId || '(none)'}`)

  if (!profileId) {
    _log(`  no profileId — falling back to full staging scan`)
    return findModByNexusId(nexusId, stagingOverride)
  }

  const modlistPath = path.join(getProfileDir(profileId), 'modlist.txt')
  _log(`  modlist path: ${modlistPath} exists=${fs.existsSync(modlistPath)}`)

  if (!fs.existsSync(modlistPath)) {
    _log(`  modlist not found — falling back to full staging scan`)
    return findModByNexusId(nexusId, stagingOverride)
  }

  const stagingRoot = getStagingRoot(stagingOverride)
  const idStr       = String(nexusId)

  let enabledIds
  try {
    const raw = fs.readFileSync(modlistPath, 'utf8')
    enabledIds = raw.split(/\r?\n/).filter(l => l.startsWith('+')).map(l => l.slice(1))
    _log(`  modlist enabled entries (${enabledIds.length}): ${enabledIds.join(', ')}`)
  } catch (err) {
    _log(`  modlist read error: ${err.message} — falling back to full staging scan`)
    return findModByNexusId(nexusId, stagingOverride)
  }

  for (const modId of enabledIds) {
    // ── 1. meta.ini modid ────────────────────────────────────────────────────
    const metaPath = path.join(stagingRoot, modId, 'meta.ini')
    if (fs.existsSync(metaPath)) {
      try {
        const content = fs.readFileSync(metaPath, 'utf8')
        const match   = content.match(/^modid\s*=\s*(\d+)/im)
        _log(`  ${modId}/meta.ini → modid=${match ? match[1] : '(not found)'}`)
        if (match && parseInt(match[1], 10) === nexusId) {
          _log(`  MATCH via modlist+meta.ini: ${modId}`)
          return modId
        }
      } catch (err) { _log(`  ${modId}/meta.ini error: ${err.message}`) }
    } else {
      _log(`  ${modId} has no meta.ini`)
    }

    // ── 2. Folder name prefix ─────────────────────────────────────────────────
    if (modId.startsWith(idStr + '-') || modId === idStr) {
      _log(`  MATCH via folder name prefix: ${modId}`)
      return modId
    }
  }

  _log(`  not found in modlist — falling back to full staging scan`)
  return findModByNexusId(nexusId, stagingOverride)
}

// ── Nexus mod polling ─────────────────────────────────────────────────────────

/**
 * Scan the Vortex downloads directory for an archive belonging to the given
 * Nexus mod ID.  Vortex names downloaded files:
 *   {mod display name}-{nexusId}-{version}-{timestamp}.{ext}
 * so we look for any filename that contains "-{nexusId}-".
 *
 * @param {number} nexusId
 * @returns {string|null}  Filename if found, otherwise null
 */
/**
 * Extract a downloaded Nexus mod archive from Vortex's downloads folder directly
 * into the staging directory, then write meta.ini so Vortex and our detection
 * code can identify the mod by its nexusId.
 *
 * This mirrors what Vortex's own "Install" button does, letting the launcher
 * handle the extraction step without requiring user interaction in Vortex.
 *
 * Returns the staging folder name on success, or null if the archive isn't
 * present / isn't yet complete (AdmZip will throw on a partial zip).
 *
 * @param {number} nexusId
 * @param {string} modName        Written into meta.ini as the display name
 * @param {string} [stagingOverride]
 * @returns {string|null}
 */
/**
 * Return true if the zip archive at archivePath contains a FOMOD installer.
 * FOMOD zips have a "fomod/" directory at the root with ModuleConfig.xml or Info.xml.
 * These mods require Vortex's install wizard — we must not extract them directly.
 *
 * @param {string} archivePath
 * @returns {boolean}
 */
function hasFomodInstaller(archivePath) {
  try {
    const AdmZip  = require('adm-zip')
    const zip     = new AdmZip(archivePath)
    return zip.getEntries().some(e => /^fomod\//i.test(e.entryName))
  } catch {
    return false
  }
}

function installModFromDownload(nexusId, modName, stagingOverride) {
  const dlRoot      = getDownloadsRoot()
  const stagingRoot = getStagingRoot(stagingOverride)
  const needle      = `-${nexusId}-`

  // Find the archive — prefer the plain .zip over numbered duplicates (.1.zip etc.)
  let archiveName
  try {
    const matches = fs.readdirSync(dlRoot, { withFileTypes: true })
      .filter(e => !e.isDirectory() && e.name.includes(needle))
      .sort((a, b) => {
        const aExtra = /\.\d+\.[^.]+$/.test(a.name) ? 1 : 0
        const bExtra = /\.\d+\.[^.]+$/.test(b.name) ? 1 : 0
        return aExtra - bExtra
      })
    archiveName = matches[0]?.name
  } catch (err) {
    _log(`installModFromDownload(${nexusId}): readdirSync failed: ${err.message}`)
    return null
  }

  if (!archiveName) {
    _log(`installModFromDownload(${nexusId}): no archive found`)
    return { stagingId: null, fomod: false }
  }

  const archivePath       = path.join(dlRoot, archiveName)
  // Strip any duplicate suffix (.1.zip → remove the .1 part)
  const stagingFolderName = archiveName.replace(/(\.\d+)?(\.[^.]+)$/, '')
  const stagingDir        = path.join(stagingRoot, stagingFolderName)

  _log(`installModFromDownload(${nexusId}): archive=${archiveName} → stagingDir=${stagingDir}`)

  if (fs.existsSync(stagingDir)) {
    _log(`installModFromDownload(${nexusId}): staging folder already exists`)
    return { stagingId: stagingFolderName, fomod: false }
  }

  // FOMOD mods need Vortex's install wizard — never extract them directly
  if (hasFomodInstaller(archivePath)) {
    _log(`installModFromDownload(${nexusId}): FOMOD installer detected — skipping auto-extract`)
    return { stagingId: null, fomod: true }
  }

  try {
    const AdmZip = require('adm-zip')
    const zip    = new AdmZip(archivePath)
    fs.mkdirSync(stagingDir, { recursive: true })
    zip.extractAllTo(stagingDir, /* overwrite */ true)
    _log(`installModFromDownload(${nexusId}): extracted ${zip.getEntries().length} entries`)
  } catch (err) {
    _log(`installModFromDownload(${nexusId}): extraction failed (${err.message}) — archive may still be downloading`)
    try { fs.rmSync(stagingDir, { recursive: true, force: true }) } catch {}
    return { stagingId: null, fomod: false }
  }

  // Write meta.ini so Vortex recognises the folder and our modid scan finds it
  try {
    fs.writeFileSync(path.join(stagingDir, 'meta.ini'), [
      '[General]',
      `gameName=${GAME_ID}`,
      `modid=${nexusId}`,
      `name=${modName || stagingFolderName}`,
      `installTime=${new Date().toISOString()}`,
      'source=nexus',
      '',
    ].join('\r\n'))
  } catch (err) {
    _log(`installModFromDownload(${nexusId}): meta.ini write failed: ${err.message}`)
  }

  return { stagingId: stagingFolderName, fomod: false }
}

function findDownloadByNexusId(nexusId) {
  const dlRoot = getDownloadsRoot()
  const needle = `-${nexusId}-`
  _log(`findDownloadByNexusId(${nexusId}) dlRoot=${dlRoot} needle="${needle}"`)

  if (!fs.existsSync(dlRoot)) {
    _log(`  downloads root does not exist`)
    return null
  }

  let files
  try { files = fs.readdirSync(dlRoot, { withFileTypes: true }).filter(e => !e.isDirectory()) }
  catch (err) { _log(`  readdirSync failed: ${err.message}`); return null }

  _log(`  scanning ${files.length} file(s): ${files.map(f => f.name).join(', ')}`)

  for (const entry of files) {
    if (entry.name.includes(needle)) {
      _log(`  MATCH: ${entry.name}`)
      return entry.name
    }
  }

  _log(`  no download found for nexusId=${nexusId}`)
  return null
}

/**
 * Poll until every mod in missingMods has been installed into the Vortex
 * staging directory.  Reports three distinct states per tick so the UI can
 * give the user clear instructions:
 *
 *   waiting    — nxm:// fired but archive not in downloads yet
 *   downloaded — archive present in downloads but not extracted to staging
 *                → user must click "Install" inside Vortex
 *   staged     — staging folder exists → fully installed
 *
 * The promise resolves with the staging folder names once ALL mods are staged.
 *
 * @param {{ nexusId: number, name: string }[]} missingMods
 * @param {string}   [stagingOverride]
 * @param {(staged: number, total: number, message: string) => void} [onProgress]
 * @param {AbortSignal} [signal]
 * @param {number}   [intervalMs=4000]
 * @param {number}   [timeoutMs=300000]
 * @param {string}   [profileId]  Active Vortex profile — enables modlist-based detection
 * @returns {Promise<string[]>}
 */
function waitForModsInStaging(missingMods, stagingOverride, onProgress, signal, intervalMs = 4000, timeoutMs = 300_000, profileId = null) {
  const deadline = Date.now() + timeoutMs

  return new Promise((resolve, reject) => {
    function tick() {
      if (signal?.aborted) return reject(new Error('Cancelled'))
      if (Date.now() > deadline) {
        return reject(new Error(
          `Timed out waiting for Vortex to install ${missingMods.length} mod(s).`
        ))
      }

      const staged     = []
      const downloaded = []   // archive present but still writing (partial download)
      const needsSetup = []   // FOMOD mod — user must install through Vortex
      const waiting    = []   // not even downloaded yet

      for (const mod of missingMods) {
        // Use profile modlist as source of truth when available; full scan as fallback
        const stagingId = findEnabledModByNexusId(mod.nexusId, profileId, stagingOverride)
        if (stagingId) {
          staged.push({ mod, stagingId })
        } else if (findDownloadByNexusId(mod.nexusId)) {
          // Archive present — try to extract to staging ourselves
          _log(`tick: ${mod.name} downloaded, attempting extraction…`)
          const result = installModFromDownload(mod.nexusId, mod.name, stagingOverride)
          if (result.stagingId) {
            _log(`tick: ${mod.name} extracted → ${result.stagingId}`)
            staged.push({ mod, stagingId: result.stagingId })
          } else if (result.fomod) {
            // FOMOD mod — needs Vortex's install wizard, then we detect the result
            needsSetup.push(mod)
          } else {
            // Archive still being written (partial download) — retry next tick
            downloaded.push(mod)
          }
        } else {
          waiting.push(mod)
        }
      }

      if (onProgress) {
        let message
        if (needsSetup.length > 0) {
          // FOMOD mods — user must run Vortex's install wizard, then we detect the result
          message = `Install in Vortex (setup required): ${needsSetup.map(m => m.name).join(', ')}`
        } else if (downloaded.length > 0) {
          message = `Waiting for download to finish: ${downloaded.map(m => m.name).join(', ')}`
        } else if (waiting.length > 0) {
          message = `Downloading ${waiting[0].name}…`
        } else {
          message = staged[staged.length - 1]?.mod.name ?? missingMods[0].name
        }
        onProgress(staged.length, missingMods.length, message)
      }

      if (staged.length === missingMods.length) {
        return resolve(staged.map(s => s.stagingId))
      }

      setTimeout(tick, intervalMs)
    }

    setTimeout(tick, intervalMs)
  })
}

// ── Status ────────────────────────────────────────────────────────────────────

/**
 * Return a snapshot of the Vortex integration state for the UI.
 */
function getStatus(vortexPath, profileId) {
  const dataPath = getDataPath()
  return {
    hasVortexExe:  !!(vortexPath && fs.existsSync(vortexPath)),
    hasVortexData: fs.existsSync(dataPath),
    hasProfile:    !!(profileId && fs.existsSync(getProfileDir(profileId))),
    profiles:      fs.existsSync(getProfilesRoot()) ? listProfiles() : [],
  }
}

// ─────────────────────────────────────────────────────────────────────────────

module.exports = {
  setLogger,
  GAME_ID,
  getDataPath,
  getStagingRoot,
  getDownloadsRoot,
  getProfilesRoot,
  getProfileDir,
  findVortexExe,
  listProfiles,
  autoDetectProfile,
  readProfilesFromState,
  tagProfile,
  findModByNexusId,
  findEnabledModByNexusId,
  findDownloadByNexusId,
  hasFomodInstaller,
  installModFromDownload,
  getStatus,
}
