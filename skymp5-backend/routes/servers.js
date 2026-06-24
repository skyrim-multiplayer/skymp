'use strict'

const router = require('express').Router()
const config = require('../config')

// Last heartbeat received from the game server via POST /:key
let heartbeat = null

router.get('/', (_req, res) => {
  res.json([
    {
      name:    heartbeat?.name    ?? config.serverName,
      address: config.skyrimServerAddress,
      port:    config.skyrimServerPort,
      online:  heartbeat?.online  ?? null,
      maxPlayers: heartbeat?.maxPlayers ?? config.serverMaxPlayers,
      lastSeen:   heartbeat?.lastSeen   ?? null,
    },
  ])
})

// Called by the SkyMP in-game client to get the game server's host/port.
// The client sends X-Session so we also return sessionValid/allowed for UI hints,
// but the required fields are just host and port.
router.get('/:key/serverinfo', async (req, res) => {
  if (req.params.key !== config.serverMasterKey) {
    return res.status(403).json({ error: 'Invalid master key.' })
  }

  // Optional session validation for the allowed/sessionValid hints
  const { lookupSession, isDiscordWhitelisted } = require('./master-api')
  const token = req.headers['x-session']
  let sessionValid = false
  let allowed      = true

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

  res.json({
    host:        config.skyrimServerAddress,
    port:        config.skyrimServerPort,
    name:        heartbeat?.name       ?? config.serverName,
    maxPlayers:  heartbeat?.maxPlayers ?? config.serverMaxPlayers,
    offlineMode: config.serverOfflineMode,
    masterKey:   config.serverMasterKey || null,
    masterUrl:   config.masterUrl       || null,
    locked:      config.serverLocked,
    sessionValid,
    allowed,
  })
})

// Called by the SkyMP client to fetch the server's mod list.
// Returns a v1 SkyMP server manifest so the client doesn't loop on 404s.
router.get('/:key/manifest.json', (req, res) => {
  if (req.params.key !== config.serverMasterKey) {
    return res.status(403).json({ error: 'Invalid master key.' })
  }
  res.json({ versionMajor: 1, mods: [] })
})

// Called by MasterClient every 5 s: POST /api/servers/:key
// Body: { name, maxPlayers, online }
router.post('/:key', (req, res) => {
  if (req.params.key !== config.serverMasterKey) {
    return res.status(403).json({ error: 'Invalid master key.' })
  }

  const { name, maxPlayers, online } = req.body || {}
  heartbeat = {
    name:       typeof name       === 'string' ? name       : config.serverName,
    maxPlayers: typeof maxPlayers === 'number' ? maxPlayers : config.serverMaxPlayers,
    online:     typeof online     === 'number' ? online     : null,
    lastSeen:   new Date().toISOString(),
  }

  res.json({ ok: true })
})

module.exports = router
module.exports.getHeartbeat = () => heartbeat
