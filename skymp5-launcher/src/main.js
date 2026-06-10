// Load .env before anything else — only in unpackaged (dev/local) builds.
// Packaged installers use real environment variables set by the OS / process manager.
if (!require('electron').app.isPackaged) {
  require('dotenv').config()
}

const { app, BrowserWindow, ipcMain, dialog, shell } = require('electron')
const path   = require('path')
const fs     = require('fs')
const os     = require('os')
const crypto = require('crypto')
const http   = require('http')
const https  = require('https')
const { spawn } = require('child_process')
const Store  = require('electron-store')
const AdmZip = require('adm-zip')
const config = require('./config')
const vortex = require('./vortex')

const isDev = process.argv.includes('--dev')

// ── Dev logger ────────────────────────────────────────────────────────────────
const LOG_FILE = isDev ? path.join(require('os').tmpdir(), 'skyrp-install.log') : null

function log(...args) {
  const line = args.join(' ')
  console.log(line)
  if (LOG_FILE) fs.appendFileSync(LOG_FILE, line + '\n')
}

if (LOG_FILE) {
  fs.writeFileSync(LOG_FILE, `=== skyrp install log ${new Date().toISOString()} ===\n`)
  console.log('[dev] logging to', LOG_FILE)
}

// Route vortex module debug output through the same logger
vortex.setLogger(log)

// Only user-specific preferences live in the store.
const store = new Store({
  defaults: {
    skyrimPath:        '',
    username:          '',
    activeServerIndex: 0,
    cachedServers:     [],   // last-known server list fetched from /api/servers
    filesVersion:      '',   // version tag from last successful file download
    discordUser:       null,
    vortexPath:        '',
    vortexEnabled:     false,
    vortexStagingPath: '',   // empty = use Vortex default (%APPDATA%\Vortex\skyrimse\mods)
    nexusApiKey:       '',   // user's free Nexus API key — used to resolve file IDs for nxm:// links
  }
})

let win = null

function send(channel, ...args) {
  if (win && !win.isDestroyed()) win.webContents.send(channel, ...args)
}

// ── Active server helper ──────────────────────────────────────────────────────
// Returns the currently selected game server from the cached API list,
// or null if no servers have been fetched yet.
function activeServer() {
  const servers = store.get('cachedServers') || []
  if (servers.length === 0) return null
  const idx = Math.min(store.get('activeServerIndex') || 0, servers.length - 1)
  return servers[idx]
}

// ── Window ────────────────────────────────────────────────────────────────────
function createWindow() {
  win = new BrowserWindow({
    width:     1280,
    height:    720,
    minWidth:  1024,
    minHeight: 600,
    frame:     false,
    resizable: true,
    webPreferences: {
      preload:          path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration:  false,
    },
    backgroundColor: '#080503',
    show: false,
  })

  win.loadFile(path.join(__dirname, 'renderer', 'index.html'))
  win.once('ready-to-show', () => win.show())

  if (isDev) win.webContents.openDevTools({ mode: 'detach' })
}

app.whenReady().then(() => {
  createWindow()
  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow()
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit()
})

// ── Window controls ───────────────────────────────────────────────────────────
ipcMain.on('window:minimize', () => win?.minimize())
ipcMain.on('window:maximize', () => {
  if (win?.isMaximized()) win.unmaximize()
  else win?.maximize()
})
ipcMain.on('window:close', () => win?.close())

// ── Settings ──────────────────────────────────────────────────────────────────
ipcMain.handle('settings:load', async () => {
  // Refresh the server list from the backend on every load.
  // On failure we keep the previously cached list so offline launches still work.
  try {
    const fetched = await fetchJSON(`${config.apiUrl}/api/servers`)
    if (Array.isArray(fetched) && fetched.length > 0) {
      store.set('cachedServers', fetched)
    }
  } catch { /* keep existing cache */ }

  const servers = store.get('cachedServers') || []
  return {
    ...store.store,
    servers,
    multiServer: servers.length > 1,
    discordUser: store.get('discordUser') || null,
  }
})
ipcMain.handle('settings:save', (_e, data) => {
  const allowed = ['skyrimPath', 'activeServerIndex',
                   'vortexPath', 'vortexEnabled']
  const clean = {}
  for (const k of allowed) if (k in data) clean[k] = data[k]
  store.set(clean)
})

// ── Folder picker ─────────────────────────────────────────────────────────────
ipcMain.handle('dialog:openFolder', async () => {
  const result = await dialog.showOpenDialog(win, {
    properties: ['openDirectory'],
    title: 'Select Skyrim Installation Folder',
  })
  return result.canceled ? null : result.filePaths[0]
})

// ── Open external URL — http/https only ──────────────────────────────────────
ipcMain.on('open:external', (_e, url) => {
  if (typeof url === 'string' && /^https?:\/\//i.test(url)) {
    shell.openExternal(url)
  }
})

// ── News ──────────────────────────────────────────────────────────────────────
ipcMain.handle('api:news', async () => {
  try { return await fetchJSON(`${config.apiUrl}/api/news`) }
  catch { return null }
})

// ── Server status ─────────────────────────────────────────────────────────────
ipcMain.handle('api:status', async () => {
  try {
    const data = await fetchJSON(`${config.apiUrl}/api/status`)
    return { ok: true, ...data }
  } catch {
    return { ok: false }
  }
})

// ── Server info ───────────────────────────────────────────────────────────────
// Include the stored session token so the backend's session-aware `allowed`
// field reflects whether this user is on the whitelist / server lock list.
ipcMain.handle('api:serverinfo', async () => {
  const session = store.get('gameSession')
  const headers = session ? { 'x-session': session } : {}
  try { return await fetchJSON(`${config.apiUrl}/api/serverinfo`, headers) }
  catch { return null }
})

// ── Discord OAuth ─────────────────────────────────────────────────────────────

ipcMain.handle('discord:getUser', () => store.get('discordUser') || null)

ipcMain.handle('discord:logout', () => {
  store.set('discordUser',   null)
  store.set('gameProfileId', null)
  store.set('gameSession',   null)

  // Clear auth-data-no-load.js so the SkyMP in-game client reverts to showing
  // its own Discord OAuth dialog (//null is read as null by the SkyMP client).
  const skyrimPath = store.get('skyrimPath')
  if (skyrimPath) {
    const authDataPath = path.join(skyrimPath, 'Data', 'Platform', 'PluginsNoLoad', 'auth-data-no-load.js')
    try { fs.writeFileSync(authDataPath, '//null') } catch { /* file may not exist yet */ }
  }

  return { success: true }
})

ipcMain.handle('discord:login', async () => {
  const state = crypto.randomBytes(32).toString('hex')

  // Open the backend's login-discord URL in the user's default browser.
  // The backend registers the state, redirects to Discord, exchanges the code
  // on callback, and makes the result available at the /status endpoint.
  shell.openExternal(`${config.apiUrl}/api/users/login-discord?state=${state}`)

  // Poll the status endpoint until auth completes or times out (5 minutes).
  const POLL_INTERVAL_MS = 2000
  const deadline = Date.now() + 5 * 60 * 1000

  while (Date.now() < deadline) {
    await new Promise(r => setTimeout(r, POLL_INTERVAL_MS))

    let data
    try {
      data = await fetchJSON(
        `${config.apiUrl}/api/users/login-discord/status?state=${encodeURIComponent(state)}`
      )
    } catch (err) {
      if (err.statusCode === 401) continue  // still pending — keep polling
      if (err.statusCode === 403) return { success: false, error: 'Auth expired or unknown state.' }
      continue  // network blip — keep polling
    }

    // 200 OK — auth complete.
    // token is the play-session token; masterApiId is the stable numeric profileId.
    const { token, masterApiId, discordUsername, discordAvatar } = data

    const discordUser = {
      username: discordUsername || `Player ${masterApiId}`,
      tag:      discordUsername || `Player ${masterApiId}`,
      avatar:   discordAvatar   || null,
    }

    store.set('discordUser',   discordUser)
    store.set('gameProfileId', masterApiId)
    store.set('gameSession',   token)

    return { success: true, user: discordUser }
  }

  return { success: false, error: 'Login timed out — please try again.' }
})

// ── Vortex integration ────────────────────────────────────────────────────────

ipcMain.handle('vortex:detect', () => {
  const found = vortex.findVortexExe()
  return { found: !!found, path: found || '' }
})

// ── Metrics ───────────────────────────────────────────────────────────────────
ipcMain.handle('api:metrics', async () => {
  try {
    const data = await fetchJSON(`${config.apiUrl}/api/metrics`)
    return { ok: true, ...data }
  }
  catch { return { ok: false, error: 'Backend unreachable' } }
})

// ── Servers ───────────────────────────────────────────────────────────────────
ipcMain.handle('api:servers', async () => {
  try {
    const servers = await fetchJSON(`${config.apiUrl}/api/servers`)
    if (Array.isArray(servers) && servers.length > 0) store.set('cachedServers', servers)
    return servers
  } catch {
    return store.get('cachedServers') || []
  }
})

// ── Modlist ───────────────────────────────────────────────────────────────────
ipcMain.handle('api:modlist', async () => {
  try { return await fetchJSON(`${config.apiUrl}/api/modlist`) }
  catch { return null }
})

// ── Launcher update check ─────────────────────────────────────────────────────
ipcMain.handle('app:checkUpdate', async () => {
  const current = app.getVersion()
  try {
    const data = await fetchJSON(`${config.apiUrl}/api/version`)
    const latest    = data.version
    const hasUpdate = compareVersions(latest, current) > 0
    return { current, latest, hasUpdate, downloadUrl: data.downloadUrl || '' }
  } catch {
    return { current, latest: null, hasUpdate: false, downloadUrl: '' }
  }
})

// ── Launch SKSE ───────────────────────────────────────────────────────────────

// Files that must exist before we allow launching
const REQUIRED_FILES = [
  path.join('Data', 'Platform', 'Plugins', 'skymp5-client.js'),
  path.join('Data', 'SKSE', 'Plugins', 'SkyrimPlatform.dll'),
  path.join('Data', 'SKSE', 'Plugins', 'MpClientPlugin.dll'),
]

ipcMain.handle('launch:skse', async () => {
  const skyrimPath    = store.get('skyrimPath')
  const vortexEnabled = store.get('vortexEnabled')
  const vortexPath    = store.get('vortexPath')

  if (!skyrimPath) {
    return { success: false, error: 'Skyrim path not configured.' }
  }

  if (vortexEnabled) {
    if (!vortexPath || !fs.existsSync(vortexPath)) {
      return { success: false, error: 'Vortex.exe not found — open Settings and re-detect Vortex.' }
    }

    // Re-write client settings before launch so server-ip/port/gameData are current.
    const srv = activeServer()
    if (srv) {
      const serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`)
      const settingsPath = path.join(skyrimPath, 'Data', 'Platform', 'Plugins', 'skymp5-client-settings.txt')
      writeClientSettings(settingsPath, srv, serverInfo)
      log('[launch] client settings written')
    }

    // Step 1: start Vortex minimised so it is running and ready to deploy.
    // The user manages their own collection/profile inside Vortex.
    log('[launch] starting Vortex minimised…')
    spawn(vortexPath, ['--start-minimized'], {
      detached: true,
      stdio: 'ignore',
    }).unref()

    // Step 2: wait 5 seconds for Vortex to finish deploying mods via hardlinks.
    await new Promise(r => setTimeout(r, 5000))
    log('[launch] 5 s elapsed — launching SKSE')

    // Step 3: launch the game directly.  Vortex uses hardlinks/symlinks so the mod
    // files are already in place in the Skyrim directory — Vortex does not need to
    // be involved at game-start time.
    const missingFiles = REQUIRED_FILES.filter(f => !fs.existsSync(path.join(skyrimPath, f)))
    if (missingFiles.length > 0) {
      const names = missingFiles.map(f => path.basename(f)).join(', ')
      return { success: false, error: `Files missing after deploy — run "Install Modpack via Vortex" first.\nMissing: ${names}` }
    }

    try {
      const exe = path.join(skyrimPath, 'skse64_loader.exe')
      spawn(exe, [], { detached: true, stdio: 'ignore', cwd: skyrimPath }).unref()
      return { success: true }
    } catch (err) {
      return { success: false, error: err.message }
    }
  }

  // ── Non-Vortex path: direct SKSE launch ──────────────────────────────────────

  // Re-write client settings before launch so server-ip/port/gameData are current.
  const srv = activeServer()
  if (srv) {
    const serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`)
    const settingsPath = path.join(skyrimPath, 'Data', 'Platform', 'Plugins', 'skymp5-client-settings.txt')
    writeClientSettings(settingsPath, srv, serverInfo)
    log('[launch] client settings written')
  }

  // Pre-launch validation
  const missingFiles = REQUIRED_FILES.filter(f => !fs.existsSync(path.join(skyrimPath, f)))
  if (missingFiles.length > 0) {
    const names = missingFiles.map(f => path.basename(f)).join(', ')
    return { success: false, error: `Files missing — Run Install first.\nMissing: ${names}` }
  }

  const exe = path.join(skyrimPath, 'skse64_loader.exe')
  try {
    spawn(exe, [], { detached: true, stdio: 'ignore', cwd: skyrimPath }).unref()
    return { success: true }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

// ── Install files ─────────────────────────────────────────────────────────────

let installing = false

ipcMain.on('install:start', (_e, mode) => {
  if (installing) return
  installing = true

  let fn
  if (mode === 'client') {
    fn = runDirectInstall()
  } else if (mode === 'vortex') {
    fn = runVortexInstall()
  } else {
    // Auto mode (used by the Play button) — delegate based on vortexEnabled setting
    const vortexEnabled = store.get('vortexEnabled')
    fn = vortexEnabled ? runVortexInstall() : runDirectInstall()
  }
  fn.catch(err => {
    log('[install] Unhandled error:', err.message)
    send('install:complete', { success: false, error: `Unexpected error: ${err.message}` })
    installing = false
  })
})

// ── Shared download + extract helpers ─────────────────────────────────────────

/**
 * Stream the client zip from the backend to a local temp file.
 * Calls onProgress(bytesReceived, totalBytes) as data arrives.
 */
function downloadClientZip(tempPath, onProgress) {
  const url = `${config.apiUrl}/api/files/zip`
  return new Promise((resolve, reject) => {
    const mod = url.startsWith('https') ? https : http
    const req = mod.get(url, res => {
      if (res.statusCode === 404) {
        res.resume()
        return reject(new Error('Update package not found on server. Run npm run merge on the backend.'))
      }
      if (res.statusCode < 200 || res.statusCode >= 300) {
        res.resume()
        return reject(new Error(`Server returned HTTP ${res.statusCode}`))
      }

      const total    = parseInt(res.headers['content-length'] || '0', 10)
      let   received = 0

      const file = fs.createWriteStream(tempPath)
      res.on('data', chunk => {
        received += chunk.length
        if (onProgress) onProgress(received, total)
      })
      res.pipe(file)
      file.on('finish', () => file.close(resolve))
      file.on('error', err => { try { fs.unlinkSync(tempPath) } catch {} reject(err) })
      res.on('error',  err => { try { fs.unlinkSync(tempPath) } catch {} reject(err) })
    })
    req.on('error', reject)
    req.setTimeout(60_000, () => { req.destroy(); reject(new Error('Download timed out')) })
  })
}

/**
 * Extract the zip at zipPath into destDir, preserving the internal path structure.
 * Calls onProgress(entryName, index, total) for each file entry.
 * Returns the number of files extracted.
 */
function extractClientZip(zipPath, destDir, onProgress) {
  const zip     = new AdmZip(zipPath)
  const entries = zip.getEntries().filter(e => !e.isDirectory)
  const total   = entries.length

  for (let i = 0; i < total; i++) {
    const entry = entries[i]
    zip.extractEntryTo(entry.entryName, destDir, /* maintainEntryPath */ true, /* overwrite */ true)
    if (onProgress) onProgress(entry.entryName, i + 1, total)
  }

  return total
}

// ── Direct install (no Vortex) ────────────────────────────────────────────────

async function runDirectInstall() {
  const abort = (msg) => {
    log('[install] ABORT:', msg)
    send('install:complete', { success: false, error: msg })
    installing = false
  }

  const skyrimPath = store.get('skyrimPath')
  if (!skyrimPath) return abort('Skyrim path not configured.')

  const srv = activeServer()
  if (!srv) return abort('No server selected — open Settings and choose a server.')

  const tempZip = path.join(os.tmpdir(), 'skyrp-client.zip')

  // Fetch serverinfo once so writeClientSettings knows offline vs online mode.
  let serverInfo = null
  try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}

  const clientSettingsPath = path.join(skyrimPath, 'Data', 'Platform', 'Plugins', 'skymp5-client-settings.txt')

  try {
    // ── 1. Check whether a download is needed ────────────────────────────────
    let serverVersion = null
    try {
      const vd = await fetchJSON(`${config.apiUrl}/api/files/version`)
      serverVersion = vd.version
    } catch (err) {
      if (err.statusCode === 404) {
        return abort('Client files have not been packaged on the server yet. Ask the server admin to run `npm run merge`.')
      }
      // Network error — play on cached files if they exist
      const allPresent = REQUIRED_FILES.every(f => fs.existsSync(path.join(skyrimPath, f)))
      if (!allPresent) return abort('Backend unreachable and client files are not installed. Check your connection.')
      log('[install] Backend unreachable - files already installed, updating settings only')
      writeClientSettings(clientSettingsPath, srv, serverInfo)
      send('install:complete', { success: true, upToDate: true })
      return
    }

    const allPresent    = REQUIRED_FILES.every(f => fs.existsSync(path.join(skyrimPath, f)))
    const needsDownload = serverVersion !== store.get('filesVersion') || !allPresent

    if (!needsDownload) {
      log('[install] Files up to date, updating settings only')
      writeClientSettings(clientSettingsPath, srv, serverInfo)
      send('install:complete', { success: true, upToDate: true })
      return
    }

    // ── 2. Download ──────────────────────────────────────────────────────────
    send('install:progress', { phase: 'download', file: 'Connecting to server…', index: 0, total: 0, skipped: false })
    await downloadClientZip(tempZip, (received, total) => {
      const mb  = n => (n / 1024 / 1024).toFixed(1)
      const pct = total > 0 ? ` (${Math.round(received / total * 100)}%)` : ''
      log(`[install] download ${mb(received)}/${mb(total)} MB`)
      send('install:progress', {
        phase: 'download',
        file:  `Downloading update… ${mb(received)} / ${mb(total)} MB${pct}`,
        index: received, total, skipped: false,
      })
    })

    // ── 3. Extract directly into Skyrim directory ────────────────────────────
    const extracted = extractClientZip(tempZip, skyrimPath, (file, i, total) => {
      log(`[install] extract [${i}/${total}] ${file}`)
      send('install:progress', { phase: 'extract', file, index: i, total, skipped: false })
    })
    log(`[install] extracted ${extracted} files`)

    // ── 4. Write server settings ─────────────────────────────────────────────
    writeClientSettings(clientSettingsPath, srv, serverInfo)

    store.set('filesVersion', serverVersion)

    send('install:complete', { success: true })
  } catch (err) {
    abort(`Install failed: ${err.message}`)
  } finally {
    try { fs.unlinkSync(tempZip) } catch {}
    installing = false
  }
}

// ── Vortex install ────────────────────────────────────────────────────────────

async function runVortexInstall() {
  const abort = (msg) => {
    log('[vortex-install] ABORT:', msg)
    send('install:complete', { success: false, error: msg })
    installing = false
  }

  const skyrimPath = store.get('skyrimPath')
  if (!skyrimPath) return abort('Skyrim path not configured.')

  const srv = activeServer()
  if (!srv) return abort('No server selected — open Settings and choose a server.')

  const tempZip = path.join(os.tmpdir(), 'skyrp-client.zip')

  // Fetch serverinfo once so writeClientSettings knows offline vs online mode.
  let serverInfo = null
  try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}

  const clientSettingsPath = path.join(
    skyrimPath,
    'Data',
    'Platform',
    'Plugins',
    'skymp5-client-settings.txt'
  )

  try {
    // ── 1. Open collection in Vortex ──────────────────────────────────────────
    try {
      const modlistData = await fetchJSON(`${config.apiUrl}/api/modlist`)
      const vortexExe   = store.get('vortexPath')

      if (Array.isArray(modlistData)) {
        const collection = modlistData.find(m => m.source === 'collection')
        if (collection?.collectionSlug) {
          // Ensure Vortex is running before opening the NXM link
          if (vortexExe && fs.existsSync(vortexExe)) {
            spawn(vortexExe, ['--game', 'skyrimse'], {
              detached: true,
              stdio: 'ignore',
            }).unref()
            await new Promise(r => setTimeout(r, 1500))
          }

          if (collection.revisionId) {
            shell.openExternal(
              `nxm://skyrimspecialedition/collections/${collection.collectionSlug}/revisions/${collection.revisionId}`
            )
            log(`[vortex-install] opened collection ${collection.collectionSlug} rev ${collection.revisionId}`)
          } else {
            shell.openExternal(
              `https://www.nexusmods.com/skyrimspecialedition/collections/${collection.collectionSlug}`
            )
            log(`[vortex-install] opened collection page (no revisionId): ${collection.collectionSlug}`)
          }
        } else {
          log('[vortex-install] no collection entry in modlist — skipping collection open')
        }
      }
    } catch {
      log('[vortex-install] could not fetch modlist, skipping collection sync')
    }

    // ── 2. Backend file handling (UNCHANGED) ──────────────────────────────────
    let serverVersion = null
    try {
      const vd = await fetchJSON(`${config.apiUrl}/api/files/version`)
      serverVersion = vd.version
    } catch (err) {
      if (err.statusCode === 404) {
        return abort(
          'Client files have not been packaged on the server yet. Ask the server admin to run `npm run merge`.'
        )
      }

      const allPresent = REQUIRED_FILES.every(f =>
        fs.existsSync(path.join(skyrimPath, f))
      )

      if (!allPresent)
        return abort(
          'Backend unreachable and client files are not installed. Check your connection.'
        )

      log('[vortex-install] Backend unreachable — files present, updating settings only')

      writeClientSettings(clientSettingsPath, srv, serverInfo)

      send('install:complete', {
        success: true,
        upToDate: true,
        vortex: true,
      })
      return
    }

    const allPresent = REQUIRED_FILES.every(f =>
      fs.existsSync(path.join(skyrimPath, f))
    )

    const needsDownload =
      serverVersion !== store.get('filesVersion') || !allPresent

    if (!needsDownload) {
      log('[vortex-install] Backend files up to date — updating settings only')

      writeClientSettings(clientSettingsPath, srv, serverInfo)

      send('install:complete', {
        success: true,
        upToDate: true,
        vortex: true,
      })
      return
    }

    // ── 3. Download backend zip ───────────────────────────────────────────────
    send('install:progress', {
      phase: 'download',
      file: 'Connecting to server…',
      index: 0,
      total: 0,
      skipped: false,
    })

    await downloadClientZip(tempZip, (received, total) => {
      const mb = n => (n / 1024 / 1024).toFixed(1)
      const pct = total > 0 ? ` (${Math.round(received / total * 100)}%)` : ''

      send('install:progress', {
        phase: 'download',
        file: `Downloading update… ${mb(received)} / ${mb(total)} MB${pct}`,
        index: received,
        total,
        skipped: false,
      })
    })

    // ── 4. Extract ───────────────────────────────────────────────────────────
    const extracted = extractClientZip(tempZip, skyrimPath, (file, i, total) => {
      log(`[vortex-install] extract [${i}/${total}] ${file}`)
      send('install:progress', {
        phase: 'extract',
        file,
        index: i,
        total,
        skipped: false,
      })
    })

    log(`[vortex-install] extracted ${extracted} files`)

    // ── 5. Settings ──────────────────────────────────────────────────────────
    writeClientSettings(clientSettingsPath, srv, serverInfo)
    log('[vortex-install] settings written')

    store.set('filesVersion', serverVersion)

    send('install:complete', { success: true, vortex: true })
  } catch (err) {
    abort(`Install failed: ${err.message}`)
  } finally {
    try { fs.unlinkSync(tempZip) } catch {}
    installing = false
  }
}

// ── Helpers ───────────────────────────────────────────────────────────────────

/**
 * Write the SkyMP client settings file (skymp5-client-settings.txt).
 *
 * Format per SkyMP docs:
 *
 *   Offline mode (server offlineMode: true):
 *     { "server-ip": "...", "server-port": N,
 *       "master": "", "server-master-key": null,
 *       "gameData": { "profileId": <integer> } }
 *
 *   Online mode (server offlineMode: false):
 *     { "server-ip": "...", "server-port": N,
 *       "master": "<masterUrl>", "server-master-key": "<masterKey>" }
 *     Also writes PluginsNoLoad/auth-data-no-load.js so the SkyMP in-game client
 *     finds pre-existing credentials and skips its own Discord OAuth dialog.
 *
 * @param {string} destPath   Absolute path to skymp5-client-settings.txt
 * @param {object} srv        Active server entry { address, port }
 * @param {object} serverInfo Cached serverinfo { offlineMode, masterKey, masterUrl }
 */
function writeClientSettings(destPath, srv, serverInfo) {
  // Start fresh every time — do not preserve stale keys from previous writes.
  const settings = {}

  settings['server-ip']   = srv.address
  settings['server-port'] = Number(srv.port)

  // Default to false (online mode) when serverInfo is unavailable — safer
  // than defaulting to offline, which would write a wrong profileId-based gameData.
  const offlineMode = serverInfo?.offlineMode ?? false

  settings['master']            = serverInfo?.masterUrl || ''
  settings['server-master-key'] = serverInfo?.masterKey || null

  if (offlineMode) {
    const profileId = store.get('gameProfileId')
    if (profileId == null) throw new Error('No profileId in store — login with Discord before playing')
    settings['gameData'] = { profileId }
  } else {
    // Write auth-data-no-load.js so the SkyMP in-game client finds pre-existing
    // credentials and skips its own Discord OAuth dialog.
    // The SkyMP client reads: {skyrimPath}/Data/Platform/PluginsNoLoad/auth-data-no-load.js
    // Format: //<RemoteAuthGameData JSON>
    // Shape:  { session, masterApiId, discordUsername, discordDiscriminator, discordAvatar }
    const session     = store.get('gameSession')
    const discordUser = store.get('discordUser')
    const profileId   = store.get('gameProfileId')
    if (session && discordUser && profileId != null) {
      const authDataPath = path.join(path.dirname(destPath), '..', 'PluginsNoLoad', 'auth-data-no-load.js')
      const authData = {
        session,
        masterApiId:          profileId,
        discordUsername:      discordUser.username || discordUser.tag || null,
        discordDiscriminator: null,
        discordAvatar:        discordUser.avatar   || null,
      }
      try {
        fs.mkdirSync(path.dirname(authDataPath), { recursive: true })
        fs.writeFileSync(authDataPath, '//' + JSON.stringify(authData))
        log('[writeClientSettings] auth-data-no-load.js written for', discordUser.username || profileId)
      } catch (err) {
        log('[writeClientSettings] Failed to write auth-data-no-load.js:', err.message)
      }
    }
  }

  fs.mkdirSync(path.dirname(destPath), { recursive: true })
  fs.writeFileSync(destPath, JSON.stringify(settings, null, 2) + '\n')
}

/**
 * Fetch the latest main-file ID for a Nexus mod using the Nexus API.
 * Returns the numeric fileId or null on failure.
 */
function fetchNexusFileId(nexusId, apiKey) {
  return new Promise(resolve => {
    const opts = {
      hostname: 'api.nexusmods.com',
      path:     `/v1/games/skyrimspecialedition/mods/${nexusId}/files.json?category=main`,
      headers:  {
        apikey:           apiKey,
        'User-Agent':     'SkyRP-Launcher/1.0.0',
        accept:           'application/json',
      },
    }
    const req = https.get(opts, res => {
      let data = ''
      res.on('data', c => { data += c })
      res.on('end', () => {
        try {
          const json  = JSON.parse(data)
          const files = json.files || []
          // Pick the newest main file (highest file_id)
          const main  = files
            .filter(f => f.category_name === 'MAIN')
            .sort((a, b) => b.file_id - a.file_id)[0]
          resolve(main ? main.file_id : null)
        } catch { resolve(null) }
      })
    })
    req.on('error', () => resolve(null))
    req.setTimeout(10_000, () => { req.destroy(); resolve(null) })
  })
}

function fetchJSON(url, headers = {}) {
  return new Promise((resolve, reject) => {
    const mod    = url.startsWith('https') ? https : http
    const urlObj = new URL(url)
    const opts   = {
      hostname: urlObj.hostname,
      port:     urlObj.port || (url.startsWith('https') ? 443 : 80),
      path:     urlObj.pathname + urlObj.search,
      method:   'GET',
      headers,
    }
    const req = mod.request(opts, res => {
      if (res.statusCode < 200 || res.statusCode >= 300) {
        res.resume()
        const e = new Error(`HTTP ${res.statusCode} from ${url}`)
        e.statusCode = res.statusCode
        reject(e)
        return
      }
      let data = ''
      res.on('data', chunk => { data += chunk })
      res.on('end', () => {
        try { resolve(JSON.parse(data)) }
        catch (e) { reject(new Error(`Invalid JSON from ${url}: ${e.message}`)) }
      })
    })
    req.on('error', reject)
    req.setTimeout(10_000, () => {
      req.destroy()
      reject(new Error(`Request timed out: ${url}`))
    })
    req.end()
  })
}

function compareVersions(a, b) {
  const pa = String(a).split('.').map(Number)
  const pb = String(b).split('.').map(Number)
  for (let i = 0; i < Math.max(pa.length, pb.length); i++) {
    const diff = (pa[i] || 0) - (pb[i] || 0)
    if (diff !== 0) return diff
  }
  return 0
}
