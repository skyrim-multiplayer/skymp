'use strict'

/**
 * Nexus Mods API integration.
 *
 * The user supplies their personal API key (Settings → the key from
 * https://next.nexusmods.com/settings/api-keys). With it we can:
 *  - validate the key and show who's logged in (+ premium status)
 *  - resolve a mod's main file
 *  - PREMIUM ONLY: generate direct download links and pull archives
 *    straight into MO2's downloads folder, Wabbajack-style.
 *
 * Free accounts can't generate download links through the API (Nexus
 * policy) — for them the launcher falls back to opening mod pages and
 * catching the nxm:// downloads via the registered handler.
 */

const https = require('https')
const fs    = require('fs')
const path  = require('path')

const GAME       = 'skyrimspecialedition'
const USER_AGENT = 'SkyRP-Launcher/1.0.0'

// ── Logger ────────────────────────────────────────────────────────────────────
let _log = (...args) => console.log('[nexus]', ...args)
function setLogger(fn) { _log = (...args) => fn('[nexus]', ...args) }

// ── Low-level API call ────────────────────────────────────────────────────────

function apiGet(apiKey, apiPath) {
  return new Promise((resolve, reject) => {
    const req = https.get({
      hostname: 'api.nexusmods.com',
      path:     apiPath,
      headers:  { apikey: apiKey, 'User-Agent': USER_AGENT, accept: 'application/json' },
    }, res => {
      let data = ''
      res.on('data', c => { data += c })
      res.on('end', () => {
        if (res.statusCode === 401) return reject(new Error('Invalid or expired Nexus API key.'))
        if (res.statusCode === 403) return reject(new Error('Nexus refused the request (premium required?).'))
        if (res.statusCode < 200 || res.statusCode >= 300) {
          return reject(new Error(`Nexus API HTTP ${res.statusCode}`))
        }
        try { resolve(JSON.parse(data)) }
        catch (err) { reject(new Error(`Bad JSON from Nexus: ${err.message}`)) }
      })
    })
    req.on('error', reject)
    req.setTimeout(15_000, () => { req.destroy(); reject(new Error('Nexus API request timed out')) })
  })
}

// ── Account ───────────────────────────────────────────────────────────────────

/**
 * Validate an API key. Returns { name, isPremium, profileUrl } or throws.
 */
async function validateKey(apiKey) {
  const data = await apiGet(apiKey, '/v1/users/validate.json')
  return {
    name:       data.name,
    isPremium:  data.is_premium === true,
    profileUrl: data.profile_url || null,
  }
}

// ── Mod files ─────────────────────────────────────────────────────────────────

/**
 * Resolve the newest MAIN file of a mod.
 * Returns { fileId, fileName, version } or null.
 */
async function getMainFile(apiKey, nexusId) {
  const data  = await apiGet(apiKey, `/v1/games/${GAME}/mods/${nexusId}/files.json?category=main`)
  const files = (data.files || []).filter(f => f.category_name === 'MAIN')
  if (files.length === 0) return null
  const main = files.sort((a, b) => b.file_id - a.file_id)[0]
  return { fileId: main.file_id, fileName: main.file_name, version: main.version || '' }
}

/**
 * PREMIUM ONLY: generate a direct download link for a specific file.
 * Returns the first CDN URI.
 */
async function getDownloadLink(apiKey, nexusId, fileId) {
  const links = await apiGet(apiKey, `/v1/games/${GAME}/mods/${nexusId}/files/${fileId}/download_link.json`)
  if (!Array.isArray(links) || links.length === 0 || !links[0].URI) {
    throw new Error('Nexus returned no download link (premium account required).')
  }
  return links[0].URI
}

// ── Download ──────────────────────────────────────────────────────────────────

/** Stream a URL to destPath, following redirects. */
function downloadFile(url, destPath, onProgress, redirectsLeft = 5) {
  return new Promise((resolve, reject) => {
    const req = https.get(url, res => {
      if (res.statusCode >= 300 && res.statusCode < 400 && res.headers.location) {
        res.resume()
        if (redirectsLeft <= 0) return reject(new Error('Too many redirects'))
        return resolve(downloadFile(res.headers.location, destPath, onProgress, redirectsLeft - 1))
      }
      if (res.statusCode !== 200) {
        res.resume()
        return reject(new Error(`HTTP ${res.statusCode} downloading mod archive`))
      }

      const total = parseInt(res.headers['content-length'] || '0', 10)
      let received = 0
      const file = fs.createWriteStream(destPath)
      res.on('data', chunk => {
        received += chunk.length
        if (onProgress) onProgress(received, total)
      })
      res.pipe(file)
      file.on('finish', () => file.close(resolve))
      file.on('error', err => { try { fs.unlinkSync(destPath) } catch {} reject(err) })
      res.on('error',  err => { try { fs.unlinkSync(destPath) } catch {} reject(err) })
    })
    req.on('error', reject)
    req.setTimeout(120_000, () => { req.destroy(); reject(new Error('Mod download timed out')) })
  })
}

/**
 * PREMIUM ONLY: download a mod's main file into a downloads directory.
 * The archive is named MO2/Nexus-style (`…-{modId}-…`) so the download
 * watcher can also identify it. Returns the archive filename.
 *
 * @param {string} apiKey
 * @param {{ nexusId: number, name: string }} mod
 * @param {string} downloadsDir
 * @param {(received: number, total: number) => void} [onProgress]
 */
async function downloadMod(apiKey, mod, downloadsDir, onProgress) {
  const main = await getMainFile(apiKey, mod.nexusId)
  if (!main) throw new Error(`No main file found on Nexus for ${mod.name}`)

  const url = await getDownloadLink(apiKey, mod.nexusId, main.fileId)

  const ext         = path.extname(main.fileName) || '.zip'
  const base        = path.basename(main.fileName, ext)
  const archiveName = `${base}-${mod.nexusId}-${main.fileId}${ext}`
  const destPath    = path.join(downloadsDir, archiveName)

  if (fs.existsSync(destPath)) {
    _log(`${archiveName} already downloaded`)
    return archiveName
  }

  _log(`downloading ${mod.name} (${main.fileName})`)
  fs.mkdirSync(downloadsDir, { recursive: true })
  const tempPath = destPath + '.unfinished'
  await downloadFile(url, tempPath, onProgress)
  fs.renameSync(tempPath, destPath)
  return archiveName
}

// ── File resolution (version pinning / optional files) ────────────────────────

/** All files for a mod: [{ fileId, fileName, version, category }]. */
async function listFiles(apiKey, nexusId) {
  const data = await apiGet(apiKey, `/v1/games/${GAME}/mods/${nexusId}/files.json`)
  return (data.files || []).map(f => ({
    fileId:   f.file_id,
    fileName: f.file_name,
    version:  f.version || '',
    category: f.category_name,      // MAIN, OPTIONAL, UPDATE, OLD_VERSION, …
  }))
}

/**
 * Choose the main file to install, honouring an explicit `fileId` pin, then a
 * `version` match among MAIN files, then the newest MAIN file.
 */
function pickMain(files, { fileId, version } = {}) {
  if (fileId) {
    const f = files.find(x => x.fileId === Number(fileId))
    if (f) return f
  }
  const mains = files.filter(f => f.category === 'MAIN')
  if (version) {
    const v = mains.find(f => f.version === version)
    if (v) return v
  }
  const pool = mains.length ? mains : files
  return pool.slice().sort((a, b) => b.fileId - a.fileId)[0] || null
}

/**
 * Download one resolved file entry into downloadsDir. The archive is named
 * deterministically (`…-{modId}-{fileId}…`) so re-runs reuse it.
 * PREMIUM ONLY (uses the download-link API).
 */
async function downloadFileEntry(apiKey, nexusId, file, downloadsDir, onProgress) {
  const url         = await getDownloadLink(apiKey, nexusId, file.fileId)
  const ext         = path.extname(file.fileName) || '.zip'
  const base        = path.basename(file.fileName, ext)
  const archiveName = `${base}-${nexusId}-${file.fileId}${ext}`
  const destPath    = path.join(downloadsDir, archiveName)

  if (fs.existsSync(destPath)) { _log(`${archiveName} already downloaded`); return archiveName }
  fs.mkdirSync(downloadsDir, { recursive: true })
  const tmp = destPath + '.unfinished'
  await downloadFile(url, tmp, onProgress)
  fs.renameSync(tmp, destPath)
  return archiveName
}

// ── SSO login (one-click, Vortex/Wabbajack-style) ─────────────────────────────

/**
 * Nexus SSO flow: connect to wss://sso.nexusmods.com, hand the browser an
 * authorize URL, and receive the user's API key over the websocket once
 * they click "Authorise" on the Nexus site.
 *
 * Requires an application slug registered with Nexus Mods. Uses Node's
 * built-in WebSocket client (Node >= 21 / current Electron).
 *
 * @param {string} appSlug                Registered Nexus application slug
 * @param {(url: string) => void} openUrl Called with the authorize URL to open
 * @param {number} [timeoutMs]
 * @returns {Promise<string>}             The user's API key
 */
function ssoLogin(appSlug, openUrl, timeoutMs = 5 * 60 * 1000) {
  return new Promise((resolve, reject) => {
    if (!appSlug) return reject(new Error('No Nexus application slug configured.'))
    if (typeof WebSocket === 'undefined') {
      return reject(new Error('WebSocket client unavailable in this runtime.'))
    }

    const id = require('crypto').randomUUID()
    const ws = new WebSocket('wss://sso.nexusmods.com')

    let settled = false
    const finish = (err, key) => {
      if (settled) return
      settled = true
      clearTimeout(timer)
      try { ws.close() } catch {}
      err ? reject(err) : resolve(key)
    }

    const timer = setTimeout(() => finish(new Error('Nexus login timed out — try again.')), timeoutMs)

    ws.onopen = () => {
      // protocol 2: server replies with a connection_token, then (after the
      // user authorises in the browser) with the api_key.
      ws.send(JSON.stringify({ id, token: null, protocol: 2 }))
      openUrl(`https://www.nexusmods.com/sso?id=${id}&application=${appSlug}`)
    }

    ws.onmessage = event => {
      let msg
      try { msg = JSON.parse(event.data) } catch { return }
      if (msg.success === false) {
        return finish(new Error(msg.error || 'Nexus SSO rejected the request.'))
      }
      if (msg.data?.api_key) {
        _log('SSO login complete')
        return finish(null, msg.data.api_key)
      }
      // First reply carries data.connection_token — nothing to do but wait.
    }

    ws.onerror = ()  => finish(new Error('Could not reach the Nexus SSO service.'))
    ws.onclose = ()  => finish(new Error('Nexus SSO connection closed before login completed.'))
  })
}

module.exports = {
  setLogger,
  validateKey,
  getMainFile,
  getDownloadLink,
  downloadMod,
  listFiles,
  pickMain,
  downloadFileEntry,
  ssoLogin,
}
