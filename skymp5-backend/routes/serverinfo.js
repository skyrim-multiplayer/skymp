const router      = require('express').Router()
const config      = require('../config')
const { lookupSession, isDiscordWhitelisted } = require('./master-api')
const { getHeartbeat }  = require('./servers')
const fs          = require('fs')
const path        = require('path')

const PUBLIC_KEYS_PATH = path.join(__dirname, '..', 'data', 'public-keys.json')

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
    // lockedAllowList intentionally omitted — never expose the allow-list to clients.
    // Session-aware fields — only meaningful when X-Session header is present
    sessionValid,
    allowed,
    publicKeys: loadPublicKeys(),
  })
})

module.exports = router
