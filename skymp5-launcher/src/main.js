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
const mo2    = require('./mo2')
const nexus  = require('./nexus')

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

// Route module debug output through the same logger
mo2.setLogger(log)
nexus.setLogger(log)

// Only user-specific preferences live in the store.
const store = new Store({
  defaults: {
    skyrimPath:        '',
    username:          '',
    activeServerIndex: 0,
    cachedServers:     [],   // last-known server list fetched from /api/servers
    filesVersion:      '',   // version tag from last successful file download
    discordUser:       null,
    mo2Enabled:        true,   // launch the game through the managed portable MO2
    nexusApiKey:       '',     // Nexus API login
    nexusUser:         null,   // { name, isPremium } from the last validation
    isolatedGame:      true,  // play from the isolated game copy instead of skyrimPath
    gameDirPath:       '',     // legacy: pre-base-dir location of the game copy
    baseDirPath:       '',     // SkyRP base dir: MO2 root, with the game at <base>\skyrim
  }
})

mo2.setRootProvider(() => store.get('baseDirPath') || null)

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

// ── Effective game path ───────────────────────────────────────────────────────
// Creates an isolated copy, this keeps the base directory clean
function isolatedGameDir() {
  const base = store.get('baseDirPath')
  if (base) return path.join(base, 'skyrim')
  // Legacy layouts from before the base-dir structure
  const legacy = store.get('gameDirPath')
  if (legacy) return legacy
  const local = process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local')
  return path.join(local, 'SkyRP', 'GameDir')
}

function isolatedGameReady() {
  return fs.existsSync(path.join(isolatedGameDir(), 'SkyrimSE.exe'))
}

function effectiveGamePath() {
  if (store.get('isolatedGame') && isolatedGameReady()) return isolatedGameDir()
  return store.get('skyrimPath')
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
  const allowed = ['skyrimPath', 'activeServerIndex', 'mo2Enabled', 'isolatedGame']
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
  try {
    const items = await fetchJSON(`${config.apiUrl}/api/news`)
    return { ok: true, items: Array.isArray(items) ? items : [] }
  } catch (err) {
    return { ok: false, error: err.message }
  }
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
  const skyrimPath = effectiveGamePath()
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

// ── MO2 integration ───────────────────────────────────────────────────────────

ipcMain.handle('mo2:status', () => mo2.getStatus())

ipcMain.handle('mo2:setup', async () => {
  try {
    await mo2.ensureInstalled(msg => send('mo2:progress', msg))

    const skyrimPath = effectiveGamePath()
    if (!skyrimPath) {
      return { success: false, error: 'Set the Skyrim path first, then run MO2 setup again.' }
    }

    let serverInfo = null
    try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}

    mo2.ensureInstance(skyrimPath, serverInfo?.loadOrder)
    mo2.registerNxmHandler()
    return { success: true, ...mo2.getStatus() }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

ipcMain.handle('mo2:open', () => {
  try { mo2.openUI(); return { success: true } }
  catch (err) { return { success: false, error: err.message } }
})

// ── Nexus Mods login ──────────────────────────────────────────────────────────

ipcMain.handle('nexus:getUser', () => store.get('nexusUser') || null)

ipcMain.handle('nexus:login', async (_e, apiKey) => {
  if (typeof apiKey !== 'string' || apiKey.trim().length < 10) {
    return { success: false, error: 'Paste your personal API key from the Nexus site.' }
  }
  try {
    const user = await nexus.validateKey(apiKey.trim())
    store.set('nexusApiKey', apiKey.trim())
    store.set('nexusUser', user)
    log(`[nexus] logged in as ${user.name} (premium: ${user.isPremium})`)
    return { success: true, user }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

ipcMain.handle('nexus:logout', () => {
  store.set('nexusApiKey', '')
  store.set('nexusUser', null)
  return { success: true }
})

// One-click SSO (Vortex/Wabbajack-style). Only available once a Nexus
// application slug has been registered and set in config.js.
ipcMain.handle('nexus:ssoAvailable', () => !!config.nexusAppSlug)

ipcMain.handle('nexus:ssoLogin', async () => {
  if (!config.nexusAppSlug) {
    return { success: false, error: 'One-click login is not configured yet — paste your API key instead.' }
  }
  try {
    const apiKey = await nexus.ssoLogin(config.nexusAppSlug, url => shell.openExternal(url))
    const user   = await nexus.validateKey(apiKey)
    store.set('nexusApiKey', apiKey)
    store.set('nexusUser', user)
    log(`[nexus] SSO login as ${user.name} (premium: ${user.isPremium})`)
    return { success: true, user }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

// ── Isolated game copy ────────────────────────────────────────────────────────

ipcMain.handle('game:isolatedStatus', () => ({
  enabled: !!store.get('isolatedGame'),
  ready:   isolatedGameReady(),
  dir:     isolatedGameDir(),
  base:    store.get('baseDirPath') || '',
}))

// Checks for clean directory
function findDirtyFiles(src) {
  const offenders = []
  const dataDir   = path.join(src, 'Data')

  try {
    for (const name of fs.readdirSync(dataDir)) {
      const l = name.toLowerCase()
      if (l.startsWith('cc')) continue                       // Creation Club — official
      if (l.endsWith('.esp')) offenders.push(`Data\\${name}`)
      else if ((l.endsWith('.esm') || l.endsWith('.esl')) && !VANILLA_MASTERS.has(l)) {
        offenders.push(`Data\\${name}`)
      }
    }
  } catch { /* no Data dir — caught by the SkyrimSE.exe check */ }

  for (const dir of ['SKSE', 'Platform']) {
    if (fs.existsSync(path.join(dataDir, dir))) offenders.push(`Data\\${dir}\\`)
  }
  for (const file of ['d3d11.dll', 'dxgi.dll', 'enbseries.ini']) {
    if (fs.existsSync(path.join(src, file))) offenders.push(file)
  }

  return offenders
}

// move CC content so it doesnt load
function disableCreationClub(gameDir) {
  const dataDir     = path.join(gameDir, 'Data')
  const disabledDir = path.join(gameDir, 'DisabledCC')
  let moved = 0
  try {
    for (const name of fs.readdirSync(dataDir)) {
      if (!name.toLowerCase().startsWith('cc')) continue
      fs.mkdirSync(disabledDir, { recursive: true })
      fs.renameSync(path.join(dataDir, name), path.join(disabledDir, name))
      moved++
    }
  } catch { /* no Data dir */ }
  if (moved > 0) log(`[isolated] moved ${moved} Creation Club file(s) to DisabledCC\\`)
  return moved
}

// True if either path is the same as, or nested inside, the other.
function pathsOverlap(a, b) {
  const norm = p => path.resolve(p).replace(/[\\/]+$/, '').toLowerCase() + path.sep
  const na = norm(a), nb = norm(b)
  return na.startsWith(nb) || nb.startsWith(na)
}

ipcMain.handle('game:createIsolated', async () => {
  const src = store.get('skyrimPath')
  if (!src || !fs.existsSync(path.join(src, 'SkyrimSE.exe'))) {
    return { success: false, error: 'Set a valid Skyrim path first (SkyrimSE.exe not found).' }
  }

  // We need a clean install to make a portable skyrim, this checks to make sure its clean
  const offenders = findDirtyFiles(src)
  if (offenders.length > 0) {
    const shown = offenders.slice(0, 6).join(', ') + (offenders.length > 6 ? ', …' : '')
    return {
      success: false,
      dirty:   true,
      error:   `Your Skyrim install is not clean (found: ${shown}). ` +
               `Please reinstall or verify Skyrim to a vanilla state, then try again.`,
    }
  }

  // Ask where to install the modlist.
  const picked = await dialog.showOpenDialog(win, {
    title:       'Choose where to install SkyRP (~16 GB: MO2 + game copy)',
    buttonLabel: 'Install here',
    properties:  ['openDirectory', 'createDirectory'],
  })
  if (picked.canceled || !picked.filePaths[0]) {
    return { success: false, canceled: true, error: 'Installation cancelled.' }
  }

  let base = picked.filePaths[0]
  try {
    const entries = fs.readdirSync(base)
    if (entries.length > 0 && !fs.existsSync(path.join(base, 'portable.txt'))) {
      base = path.join(base, 'SkyRP')
    }
  } catch { /* unreadable — let later steps surface the real error */ }

  const dst = path.join(base, 'skyrim')

  // Dummy protection for those trying to install it on their base directory
  if (pathsOverlap(src, dst) || pathsOverlap(src, base)) {
    return {
      success: false,
      error: 'Choose an install location OUTSIDE your Skyrim folder. ' +
             'Portable install is for compatibility. If you lack the diskspace, turn off portable install.',
    }
  }

  try {
    store.set('baseDirPath', base)
    send('isolated:progress', 'Installing Mod Organizer 2…')
    await mo2.ensureInstalled(msg => send('isolated:progress', msg))

    // portable copy setup
    if (!fs.existsSync(path.join(dst, 'SkyrimSE.exe'))) {
      const copy = await copyGameDir(src, dst)
      if (!copy.success) return copy
    } else {
      log('[isolated] reusing existing game copy at ' + dst)
    }
    disableCreationClub(dst)

    // configuration
    let serverInfo = null
    try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}
    mo2.ensureInstance(dst, serverInfo?.loadOrder)
    mo2.registerNxmHandler()

    store.set('isolatedGame', true)
    store.set('mo2Enabled', true)

    log(`[isolated] SkyRP install ready at ${base}`)
    return { success: true, dir: base }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

// robocopy the game install; resolves { success, copied } or { success: false, error }
function copyGameDir(src, dst) {
  return new Promise(resolve => {
    // robocopy: /E all subdirs, /MT multithreaded, minimal logging, one line per copied file so we can show progress.
    const args = [src, dst, '/E', '/MT:8', '/NJH', '/NJS', '/NDL', '/NC', '/NS', '/NP']
    const child = spawn('robocopy', args, { windowsHide: true })

    let copied = 0
    child.stdout.on('data', chunk => {
      const lines = chunk.toString().split(/\r?\n/).filter(l => l.trim())
      copied += lines.length
      if (copied % 25 < lines.length) {
        send('isolated:progress', `Copying game files… ${copied} files`)
      }
    })

    child.on('error', err => resolve({ success: false, error: `robocopy failed: ${err.message}` }))
    child.on('close', code => {
      // robocopy exit codes 0–7 mean success (8+ are failures)
      if (code >= 8) return resolve({ success: false, error: `robocopy exited with code ${code}` })
      log(`[isolated] game copy complete (${copied} files) at ${dst}`)
      resolve({ success: true, copied })
    })
  })
}

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
  try {
    const items = await fetchJSON(`${config.apiUrl}/api/modlist`)
    return { ok: true, items: Array.isArray(items) ? items : [] }
  } catch (err) {
    return { ok: false, error: err.message }
  }
})

// ── Game process detection ────────────────────────────────────────────────────
// Used by the renderer to switch the Play button into its "running" state.
function isProcessRunning(imageName) {
  return new Promise(resolve => {
    require('child_process').exec(
      `tasklist /FI "IMAGENAME eq ${imageName}" /NH`,
      { timeout: 5000, windowsHide: true },
      (err, stdout) => resolve(!err && stdout.toLowerCase().includes(imageName.toLowerCase()))
    )
  })
}

// Lightweight update probe for the Play/Update button: compares the server's
// published client-files version with what was last installed.
ipcMain.handle('files:updateCheck', async () => {
  try {
    const vd = await fetchJSON(`${config.apiUrl}/api/files/version`)
    const gamePath   = effectiveGamePath()
    const allPresent = !!gamePath && REQUIRED_FILES.every(f => fs.existsSync(path.join(gamePath, f)))
    return {
      ok: true,
      updateAvailable: vd.version !== store.get('filesVersion') || !allPresent,
      serverVersion:   vd.version,
    }
  } catch {
    return { ok: false, updateAvailable: false }
  }
})

ipcMain.handle('game:isRunning', async () => {
  if (process.platform !== 'win32') return false
  return (await isProcessRunning('SkyrimSE.exe')) || (await isProcessRunning('skse64_loader.exe'))
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
  const skyrimPath = effectiveGamePath()
  const mo2Enabled = store.get('mo2Enabled')

  if (!skyrimPath) {
    return { success: false, error: 'Skyrim path not configured.' }
  }

  if (mo2Enabled && !mo2.isInstalled()) {
    return { success: false, error: 'MO2 is not set up — open Settings → Mod Manager and run setup.' }
  }

  // Shared pre-launch steps: client settings, load order, file validation.
  const prep = await prepareForLaunch(skyrimPath, mo2Enabled)
  if (!prep.success) return prep

  try {
    if (mo2Enabled) {
      // MO2 manages plugins.txt itself via the profile; launch through its VFS.
      mo2.launchGame()
    } else {
      // Direct launch (manual mod installs): run SKSE in active game dir
      const exe = path.join(skyrimPath, 'skse64_loader.exe')
      if (!fs.existsSync(exe)) {
        return { success: false, error: `skse64_loader.exe not found in ${skyrimPath}. Install SKSE there, or enable MO2.` }
      }
      spawn(exe, [], { detached: true, stdio: 'ignore', cwd: skyrimPath }).unref()
    }
    return { success: true, loadOrderFixed: prep.loadOrderFixed }
  } catch (err) {
    return { success: false, error: err.message }
  }
})

// Troubleshooting: force a launch path regardless of the mo2Enabled setting.
ipcMain.handle('launch:viaMO2', async () => {
  const skyrimPath = effectiveGamePath()
  if (!skyrimPath) return { success: false, error: 'Skyrim path not configured.' }
  if (!mo2.isInstalled()) return { success: false, error: 'MO2 is not installed — run Install Modpack first.' }
  const prep = await prepareForLaunch(skyrimPath, true)
  if (!prep.success) return prep
  try { mo2.launchGame(); return { success: true } }
  catch (err) { return { success: false, error: err.message } }
})

ipcMain.handle('launch:direct', async () => {
  const skyrimPath = effectiveGamePath()
  if (!skyrimPath) return { success: false, error: 'Skyrim path not configured.' }
  const prep = await prepareForLaunch(skyrimPath, false)
  if (!prep.success) return prep
  const exe = path.join(skyrimPath, 'skse64_loader.exe')
  if (!fs.existsSync(exe)) {
    return { success: false, error: `skse64_loader.exe not found in ${skyrimPath}. Install SKSE there first.` }
  }
  try {
    spawn(exe, [], { detached: true, stdio: 'ignore', cwd: skyrimPath }).unref()
    return { success: true }
  } catch (err) { return { success: false, error: err.message } }
})

/**
 * Common pre-launch pipeline:
 *  1. Re-write skymp5-client-settings.txt so server-ip/port/gameData are current.
 *  2. Sync plugins.txt with the server's published load order (if available).
 *     Blocks the launch when required plugins are missing from Data/.
 *  3. Verify the SkyMP client files exist.
 */
async function prepareForLaunch(skyrimPath, viaMO2) {
  const srv = activeServer()
  let serverInfo = null

  if (srv) {
    try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}
    const settingsPath = path.join(skyrimPath, 'Data', 'Platform', 'Plugins', 'skymp5-client-settings.txt')
    try {
      writeClientSettings(settingsPath, srv, serverInfo)
      log('[launch] client settings written')
    } catch (err) {
      return { success: false, error: err.message }
    }
  }

  // ── Load order sync ──────────────────────────────────────────────────────────
  let loadOrderFixed = false
  if (Array.isArray(serverInfo?.loadOrder) && serverInfo.loadOrder.length > 0) {
    if (viaMO2) {
      mo2.ensureInstance(skyrimPath, serverInfo.loadOrder)
      const missing = missingPluginsForMO2(skyrimPath, serverInfo.loadOrder)
      if (missing.length > 0) {
        return {
          success: false,
          error: `Missing required plugins: ${missing.join(', ')}. ` +
                 `Run "Install Modpack" in Settings → Mod Manager first.`,
        }
      }
      loadOrderFixed = true
    } else {
      const result = fixLoadOrder(skyrimPath, serverInfo.loadOrder)
      loadOrderFixed = result.changed
      if (result.missing.length > 0) {
        return {
          success: false,
          error: `Missing required plugins: ${result.missing.join(', ')}. ` +
                 `Install the server's modlist first (see the Modlist panel).`,
        }
      }
      if (result.changed) log('[launch] plugins.txt updated to match server load order')
    }
  } else {
    log('[launch] server load order unavailable — leaving plugins.txt untouched')
  }

  // ── SKSE runtime ─────────────────────────────────────────────────────────────
  if (!fs.existsSync(path.join(skyrimPath, 'skse64_loader.exe'))) {
    return {
      success: false,
      error: 'SKSE is not installed (skse64_loader.exe missing). ' +
             'Install the modpack, or place SKSE in your game folder.',
    }
  }

  // ── Required client files ────────────────────────────────────────────────────
  const missingFiles = REQUIRED_FILES.filter(f => !fs.existsSync(path.join(skyrimPath, f)))
  if (missingFiles.length > 0) {
    const names = missingFiles.map(f => path.basename(f)).join(', ')
    const hint  = viaMO2 ? 'run "Install Modpack via MO2" first' : 'run Install first'
    return { success: false, error: `Files missing — ${hint}.\nMissing: ${names}` }
  }

  return { success: true, loadOrderFixed }
}

const VANILLA_MASTERS = new Set([
  'skyrim.esm', 'update.esm', 'dawnguard.esm', 'hearthfires.esm', 'dragonborn.esm', '_resourcepack.esl',
])

function pluginsTxtDirs() {
  const local = process.env.LOCALAPPDATA || path.join(os.homedir(), 'AppData', 'Local')
  const variants = [
    'Skyrim Special Edition',
    'Skyrim Special Edition GOG',
    'Skyrim Special Edition EPIC',
    'Skyrim Special Edition MS',
  ]
  const existing = variants.map(v => path.join(local, v)).filter(p => fs.existsSync(p))
  return existing.length > 0 ? existing : [path.join(local, variants[0])]
}

// Plugin sync
function fixLoadOrder(skyrimPath, serverLoadOrder) {
  const dataDir = path.join(skyrimPath, 'Data')

  const serverPlugins = serverLoadOrder
    .map(f => path.basename(f))
    .filter(f => !VANILLA_MASTERS.has(f.toLowerCase()))

  const missing = serverPlugins.filter(f => !fs.existsSync(path.join(dataDir, f)))
  if (missing.length > 0) return { changed: false, missing }

  const next  = serverPlugins.map(f => `*${f}`).join('\r\n') + '\r\n'
  let changed = false

  for (const dir of pluginsTxtDirs()) {
    const pluginsPath = path.join(dir, 'Plugins.txt')

    let current = null
    try { current = fs.readFileSync(pluginsPath, 'utf8') } catch {}

    if (current !== next) {
      const dropped = (current || '')
        .split(/\r?\n/)
        .filter(l => l.startsWith('*'))
        .map(l => l.slice(1).trim())
        .filter(f => f && !serverPlugins.some(p => p.toLowerCase() === f.toLowerCase()) &&
                     !VANILLA_MASTERS.has(f.toLowerCase()))
      if (dropped.length > 0) {
        log(`[launch] disabling client-side plugins (not allowed on this server): ${dropped.join(', ')}`)
      }
      fs.mkdirSync(dir, { recursive: true })
      fs.writeFileSync(pluginsPath, next)
      changed = true
      log(`[launch] wrote ${pluginsPath} (exactly ${serverPlugins.length} server plugins)`)
    }
  }

  return { changed, missing: [] }
}

function missingPluginsForMO2(skyrimPath, serverLoadOrder) {
  const dataDir = path.join(skyrimPath, 'Data')
  const modsDir = mo2.getModsDir()

  let modDirs = []
  try {
    modDirs = fs.readdirSync(modsDir, { withFileTypes: true })
      .filter(e => e.isDirectory())
      .map(e => path.join(modsDir, e.name))
  } catch {}

  return serverLoadOrder
    .map(f => path.basename(f))
    .filter(f => !VANILLA_MASTERS.has(f.toLowerCase()))
    .filter(f =>
      !fs.existsSync(path.join(dataDir, f)) &&
      !modDirs.some(dir => fs.existsSync(path.join(dir, f))))
}

// ── Install files ─────────────────────────────────────────────────────────────

let installing = false

ipcMain.on('install:start', (_e, mode) => {
  if (installing) return
  installing = true

  let fn
  if (mode === 'client') {
    fn = runDirectInstall()
  } else if (mode === 'mo2') {
    fn = runMO2Install()
  } else {
    // Auto mode (used by the Play button) — delegate based on mo2Enabled setting
    fn = store.get('mo2Enabled') ? runMO2Install() : runDirectInstall()
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

// ── Client files install core ─────────────────────────────────────────────────
// Shared by the direct and MO2 installers: version check, download, extract, client settings.

async function installClientFilesCore(skyrimPath, srv, serverInfo) {
  const tempZip = path.join(os.tmpdir(), 'skyrp-client.zip')
  const clientSettingsPath = path.join(skyrimPath, 'Data', 'Platform', 'Plugins', 'skymp5-client-settings.txt')

  try {
    // ── 1. Check whether a download is needed ────────────────────────────────
    let serverVersion = null
    try {
      const vd = await fetchJSON(`${config.apiUrl}/api/files/version`)
      serverVersion = vd.version
    } catch (err) {
      if (err.statusCode === 404) {
        return { success: false, error: 'Client files have not been packaged on the server yet. Ask the server admin to run `npm run build-client`.' }
      }
      // Network error — play on cached files if they exist
      const allPresent = REQUIRED_FILES.every(f => fs.existsSync(path.join(skyrimPath, f)))
      if (!allPresent) return { success: false, error: 'Backend unreachable and client files are not installed. Check your connection.' }
      log('[install] Backend unreachable - files already installed, updating settings only')
      writeClientSettings(clientSettingsPath, srv, serverInfo)
      return { success: true, upToDate: true }
    }

    const allPresent    = REQUIRED_FILES.every(f => fs.existsSync(path.join(skyrimPath, f)))
    const needsDownload = serverVersion !== store.get('filesVersion') || !allPresent

    if (!needsDownload) {
      log('[install] Files up to date, updating settings only')
      writeClientSettings(clientSettingsPath, srv, serverInfo)
      return { success: true, upToDate: true }
    }

    // ── 2. Download ──────────────────────────────────────────────────────────
    send('install:progress', { phase: 'download', file: 'Connecting to server…', index: 0, total: 0, skipped: false })
    await downloadClientZip(tempZip, (received, total) => {
      const mb  = n => (n / 1024 / 1024).toFixed(1)
      const pct = total > 0 ? ` (${Math.round(received / total * 100)}%)` : ''
      send('install:progress', {
        phase: 'download',
        file:  `Downloading update… ${mb(received)} / ${mb(total)} MB${pct}`,
        index: received, total, skipped: false,
      })
    })

    // ── 3. Extract directly into Skyrim directory ────────────────────────────
    const extracted = extractClientZip(tempZip, skyrimPath, (file, i, total) => {
      send('install:progress', { phase: 'extract', file, index: i, total, skipped: false })
    })
    log(`[install] extracted ${extracted} files`)

    // ── 4. Write server settings ─────────────────────────────────────────────
    writeClientSettings(clientSettingsPath, srv, serverInfo)
    store.set('filesVersion', serverVersion)

    return { success: true }
  } catch (err) {
    return { success: false, error: `Install failed: ${err.message}` }
  } finally {
    try { fs.unlinkSync(tempZip) } catch {}
  }
}

// ── Direct install (no mod manager) ───────────────────────────────────────────

async function runDirectInstall() {
  const skyrimPath = effectiveGamePath()
  const srv        = activeServer()

  const fail = (msg) => {
    log('[install] ABORT:', msg)
    send('install:complete', { success: false, error: msg })
    installing = false
  }

  if (!skyrimPath) return fail('Skyrim path not configured.')
  if (!srv)        return fail('No server selected — open Settings and choose a server.')

  let serverInfo = null
  try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}

  const core = await installClientFilesCore(skyrimPath, srv, serverInfo)
  send('install:complete', core.success
    ? { success: true, upToDate: core.upToDate }
    : { success: false, error: core.error })
  installing = false
}

// ── MO2 install ───────────────────────────────────────────────────────────────
// Full modpack pipeline: MO2 itself → SkyMP client files → Nexus mods.

async function runMO2Install() {
  const fail = (msg) => {
    log('[mo2-install] ABORT:', msg)
    send('install:complete', { success: false, error: msg })
    installing = false
  }

  const skyrimPath = effectiveGamePath()
  if (!skyrimPath) return fail('Skyrim path not configured.')

  const srv = activeServer()
  if (!srv) return fail('No server selected — open Settings and choose a server.')

  try {
    // ── 1. MO2 itself, the portable instance, and the nxm:// handler ─────────
    await mo2.ensureInstalled(msg =>
      send('install:progress', { phase: 'download', file: msg, index: 0, total: 0, skipped: false }))

    let serverInfo = null
    try { serverInfo = await fetchJSON(`${config.apiUrl}/api/serverinfo`) } catch {}
    mo2.ensureInstance(skyrimPath, serverInfo?.loadOrder)
    mo2.registerNxmHandler()
    mo2.setGameDataDir(path.join(skyrimPath, 'Data'))

    // ── 2. SkyMP client files into the real Data/ ─────────────────────────────
    const core = await installClientFilesCore(skyrimPath, srv, serverInfo)
    if (!core.success) return fail(core.error)

    // ── 3. Nexus mods from the server modlist ─────────────────────────────────
    let modlist = []
    try { modlist = await fetchJSON(`${config.apiUrl}/api/modlist`) }
    catch (err) { return fail(`Could not fetch the server modlist: ${err.message}`) }
    if (!Array.isArray(modlist)) return fail('Server modlist has an unexpected format.')

    const collections = modlist.filter(m => m.source === 'collection')
    if (collections.length > 0) {
      log('[mo2-install] collection entries are Vortex-only — skipped:',
          collections.map(m => m.name).join(', '))
    }

    // ── 3a. URL-sourced resources (SKSE & other non-Nexus files) ─────────────
    // root: true entries put their exe/dll payload into the game root —
    // exactly how Wabbajack ships SKSE — with any Data payload as a mod.
    const urlMods = modlist.filter(m => m.source === 'url' && m.enabled && m.url)
    for (const m of urlMods) {
      try {
        if (m.root && m.checkFile && fs.existsSync(path.join(skyrimPath, m.checkFile))) {
          log(`[mo2-install] ${m.name} already present (${m.checkFile})`)
          continue
        }
        if (!m.root) {
          const existing = mo2.findModByName(m.name)
          if (existing) { mo2.enableMod(existing); continue }
        }

        let fileName
        try { fileName = decodeURIComponent(new URL(m.url).pathname.split('/').pop()) } catch {}
        if (!fileName) fileName = `${m.name.replace(/[^\w.-]/g, '_')}.7z`

        const mb = n => (n / 1024 / 1024).toFixed(1)
        send('install:progress', { phase: 'mods', file: `Downloading ${m.name}…`, index: 0, total: urlMods.length, skipped: false })
        await mo2.downloadToDownloads(m.url, fileName, (received, total) => {
          const pct = total > 0 ? ` (${Math.round(received / total * 100)}%)` : ''
          send('install:progress', { phase: 'mods', file: `Downloading ${m.name}… ${mb(received)} MB${pct}`, index: 0, total: urlMods.length, skipped: false })
        })

        send('install:progress', { phase: 'mods', file: `Installing ${m.name}…`, index: 0, total: urlMods.length, skipped: false })
        if (m.root) {
          const r = mo2.installRootArchive(fileName, skyrimPath, m.name)
          if (r.folder) mo2.enableMod(r.folder)
        } else {
          const r = mo2.installModFromArchive(fileName, null, m.name, m.exclude)
          if (r.folder) mo2.enableMod(r.folder)
        }
      } catch (err) {
        return fail(`Failed to install ${m.name}: ${err.message}`)
      }
    }

    // ── 3b. Nexus mods ────────────────────────────────────────────────────────
    const nexusMods = modlist.filter(m => m.source === 'nexus' && m.enabled && m.nexusId)

    if (nexusMods.length === 0 && urlMods.length === 0) {
      send('install:complete', {
        success: true, mo2: true, upToDate: core.upToDate, modsTotal: 0,
        warning: 'The server modlist has no Nexus entries yet — generate data/modlist.json on the backend (collection-to-modlist).',
      })
      return
    }

    // Enable anything already installed, collect what's missing
    const missing = []
    for (const m of nexusMods) {
      const existing = mo2.findModByNexusId(m.nexusId)
      if (existing) mo2.enableMod(existing)
      else missing.push(m)
    }

    if (missing.length > 0) {
      const apiKey    = store.get('nexusApiKey')
      const nexusUser = store.get('nexusUser')

      if (apiKey && nexusUser?.isPremium) {
        // ── Premium: direct download + install every mod, synchronously ──────
        const failed = []
        for (let i = 0; i < missing.length; i++) {
          const m  = missing[i]
          const mb = n => (n / 1024 / 1024).toFixed(1)
          try {
            const archiveName = await nexus.downloadMod(apiKey, m, mo2.getDownloadsDir(), (received, total) => {
              const pct = total > 0 ? ` (${Math.round(received / total * 100)}%)` : ''
              send('install:progress', {
                phase: 'mods',
                file:  `Downloading ${m.name}… ${mb(received)} / ${mb(total)} MB${pct}`,
                index: i, total: missing.length, skipped: false,
              })
            })
            send('install:progress', { phase: 'mods', file: `Installing ${m.name}…`, index: i, total: missing.length, skipped: false })
            const result = mo2.installModFromArchive(archiveName, m.nexusId, m.name, m.exclude)
            if (result.folder) {
              mo2.enableMod(result.folder)
              send('install:progress', { phase: 'mods', file: `Installed ${m.name}`, index: i + 1, total: missing.length, skipped: false })
            } else {
              const why = result.error || 'install did not complete'
              log(`[mo2-install] ${m.name}: ${why}`)
              failed.push(`${m.name} (${why})`)
            }
          } catch (err) {
            log(`[mo2-install] ${m.name}: ${err.message}`)
            failed.push(`${m.name} (${err.message})`)
          }
        }

        if (failed.length > 0) {
          return fail(`${failed.length} mod(s) failed to install: ${failed.join('; ')}`)
        }
      } else {
        // for the free nexus accounts
        for (const m of missing) {
          shell.openExternal(`https://www.nexusmods.com/skyrimspecialedition/mods/${m.nexusId}?tab=files`)
          await new Promise(r => setTimeout(r, 800))
        }

        send('install:progress', {
          phase: 'mods',
          file:  `Opened ${missing.length} Nexus page(s) — use "Mod Manager Download" on each` +
                 (apiKey ? '' : ' (log into Nexus in the launcher for automatic downloads)'),
          index: 0, total: missing.length, skipped: false,
        })

        await mo2.waitForMods(missing, (done, total, message) => {
          send('install:progress', { phase: 'mods', file: message, index: done, total, skipped: false })
        })
      }
    }

    send('install:complete', { success: true, mo2: true, upToDate: core.upToDate, modsTotal: nexusMods.length + urlMods.length })
  } catch (err) {
    fail(`Install failed: ${err.message}`)
    return
  } finally {
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
