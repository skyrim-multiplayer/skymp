const router      = require('express').Router()
const http        = require('http')
const config      = require('../config')
const { lookupSession, isDiscordWhitelisted } = require('./master-api')
const { getHeartbeat }  = require('./servers')
const fs          = require('fs')
const path        = require('path')

const PUBLIC_KEYS_PATH = path.join(__dirname, '..', 'data', 'public-keys.json')

// Load order. Checks data/manifest.json from server
let loadOrderCache = { value: null, expiresAt: 0 }

function fetchGameJson(pathname) {
  return new Promise(resolve => {
    const req = http.get(
      { host: config.skyrimServerHost, port: config.skympUiPort, path: pathname, timeout: 3000 },
      res => {
        if (res.statusCode !== 200) { res.resume(); return resolve(null) }
        let data = ''
        res.on('data', c => { data += c })
        res.on('end', () => {
          try { resolve(JSON.parse(data)) } catch { resolve(null) }
        })
      }
    )
    req.on('error',   () => resolve(null))
    req.on('timeout', () => { req.destroy(); resolve(null) })
  })
}

async function getGameLoadOrder() {
  if (loadOrderCache.expiresAt > Date.now()) return loadOrderCache.value

  const manifest = (await fetchGameJson('/manifest.json')) || (await fetchGameJson('/data/manifest.json'))
  const value = Array.isArray(manifest?.loadOrder) ? manifest.loadOrder : loadOrderCache.value
  loadOrderCache = { value, expiresAt: Date.now() + 60_000 }
  return value
}

function loadPublicKeys() {
  try { return JSON.parse(fs.readFileSync(PUBLIC_KEYS_PATH, 'utf8')) }
  catch { return null }
}

router.get('/', async (req, res) => {
  const token = req.headers['x-session']

  let sessionValid = false
  let allowed      = true   // true when no session provided (offline / launcher handles it)

  if (token) {
    const entry = lookupSession(token)
    if (!entry) {
      sessionValid = false
      allowed      = false
    } else {
      sessionValid = true
      if (config.serverLocked) {
        allowed = config.serverLockedAllowList.includes(entry.discordId)
      } else {
        try {
          allowed = await isDiscordWhitelisted(entry.discordId)
        } catch {
          allowed = false
        }
      }
    }
  }

  const hb = getHeartbeat()

  res.json({
    name:                hb?.name       ?? config.serverName,
    maxPlayers:          hb?.maxPlayers ?? config.serverMaxPlayers,
    port:                config.skyrimServerPort,
    offlineMode:         config.serverOfflineMode,
    npcEnabled:          config.serverNpcEnabled,
    gamemode:            config.serverGamemode,
    discordAuthRequired: !!config.discordClientId,
    masterKey:           config.serverMasterKey  || null,
    masterUrl:           config.masterUrl         || null,
    locked:              config.serverLocked,
    // Server's esp/esm load order (basenames, in order) — null if offline
    loadOrder:           await getGameLoadOrder(),
    // lockedAllowList intentionally omitted — never expose the allow-list to clients.
    // Session-aware fields — only meaningful when X-Session header is present
    sessionValid,
    allowed,
    publicKeys: loadPublicKeys(),
  })
})

module.exports = router
