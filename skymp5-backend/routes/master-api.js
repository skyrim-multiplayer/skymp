'use strict'

/**
 * Master API — called by the SkyMP game server (not the client directly).
 *
 * Mounted twice in server.js:
 *   app.use('/auth',        masterApiRoute)  → POST /auth/session
 *   app.use('/api/servers', masterApiRoute)  → GET/POST /api/servers/:key/…
 *
 * Endpoints:
 *
 *   POST /auth/session
 *     Body:    { discordUser: { id, username } }
 *     Returns: { profileId, session }
 *     Launcher calls this after Discord login to get a stable profileId and
 *     a session token that the game client passes to the game server.
 *
 *   GET /api/servers/:key/sessions/:session
 *     Called by the game server to validate a session token.
 *     Returns: { user: { id, discordId, username } }
 *
 *   GET /api/servers/:key/sessions/:session/balance
 *     Called by the game server to fetch a player's coin balance.
 *     Returns: { user: { id, balance } }
 *
 *   POST /api/servers/:key/sessions/:session/purchase
 *     Called by the game server (with X-Auth-Token) to spend a player's coins.
 *     Body:    { balanceToSpend: number }
 *     Returns: { balanceSpent: number, success: boolean }
 *
 *   GET /api/servers/:key/profiles/:profileId/check
 *     Called by the game server in offline mode to verify a profileId is allowed.
 *     Applies the same lock / whitelist rules as session validation.
 *     Returns: { allowed: true }  or  403/404 with { error }
 *
 *   POST /api/servers/:key/profiles/:profileId/factions
 *     Called by the game server for immersive in-game faction appointments.
 *     Requires X-Auth-Token. Body: { requirementId, playerName?, notes? }
 *
 *   DELETE /api/servers/:key/profiles/:profileId/factions/:assignmentId
 *     Called by the game server to remove one official backend faction slot.
 *     Requires X-Auth-Token.
 */

const router = require('express').Router()
const crypto = require('crypto')
const fs     = require('fs')
const path   = require('path')
const config = require('../config')
const factionWhitelist = require('../sources/factionWhitelist')
const serverAccess = require('../sources/serverAccess')
const profiles = require('../sources/profiles')
const players  = require('../sources/players')

// ── Persistent balance store — profileId → coin balance ──────────────────────

const BALANCES_PATH = path.join(__dirname, '..', 'data', 'balances.json')

function loadBalances() {
  try { return JSON.parse(fs.readFileSync(BALANCES_PATH, 'utf8')) }
  catch { return {} }
}

function saveBalances(data) {
  try { fs.writeFileSync(BALANCES_PATH, JSON.stringify(data, null, 2) + '\n') }
  catch (e) { console.error('Failed to persist balances:', e) }
}

function getBalance(profileId) {
  const data = loadBalances()
  return typeof data[profileId] === 'number' ? data[profileId] : 0
}

function setBalance(profileId, balance) {
  const data = loadBalances()
  data[profileId] = balance
  saveBalances(data)
}

// ── In-memory session store — used for online-mode validation only ────────────

const sessions      = new Map()
const SESSION_TTL   = 24 * 60 * 60 * 1000  // 24 h
const SESSIONS_PATH = path.join(__dirname, '..', 'data', 'sessions.json')

function pruneExpired() {
  const now = Date.now()
  for (const [token, s] of sessions)
    if (s.expiresAt < now) sessions.delete(token)
}

function saveSessions() {
  const now     = Date.now()
  const entries = [...sessions.entries()].filter(([, s]) => s.expiresAt > now)
  try { fs.writeFileSync(SESSIONS_PATH, JSON.stringify(entries, null, 2) + '\n') }
  catch (e) { console.error('Failed to persist sessions:', e) }
}

function loadSessions() {
  try {
    const entries = JSON.parse(fs.readFileSync(SESSIONS_PATH, 'utf8'))
    const now     = Date.now()
    for (const [token, s] of entries)
      if (s.expiresAt > now) sessions.set(token, s)
    console.log(`Loaded ${sessions.size} active session(s) from disk`)
  } catch { /* first run or file absent — start fresh */ }
}

loadSessions()

// ── Helper — look up a session entry (exported for serverinfo route) ──────────

function lookupSession(token) {
  pruneExpired()
  return sessions.get(token) || null
}

// ── Helper — validate server master key ──────────────────────────────────────

function checkKey(req, res) {
  if (req.params.key !== config.serverMasterKey) {
    res.status(403).json({ error: 'Invalid master key.' })
    return false
  }
  return true
}

function checkWriteToken(req, res) {
  const authToken = req.headers['x-auth-token']
  if (!authToken || authToken !== config.masterApiAuthToken) {
    res.status(403).json({ error: 'Invalid auth token.' })
    return false
  }
  return true
}

function getProfileDiscordId(req, res) {
  const profileId = parseInt(req.params.profileId, 10)
  if (isNaN(profileId)) {
    res.status(400).json({ error: 'Invalid profileId.' })
    return null
  }

  const discordId = profiles.getDiscordIdByProfileId(profileId)
  if (!discordId) {
    res.status(404).json({ error: 'profileNotFound' })
    return null
  }

  return discordId
}

function getProfileFactionPayload(discordId) {
  return {
    permissions: factionWhitelist.getPlayerFactionPermissions(discordId),
    gameFactions: factionWhitelist.getPlayerGameFactions(discordId),
    factions: factionWhitelist.getPlayerAssignments(discordId),
  }
}

// ── Session creation helper (used by POST /auth/session and discord-auth callback) ─

function createSession(discordUser) {
  pruneExpired()
  const player = players.upsertFromDiscordUser(discordUser)
  const profileId = player.profileId
  const token = crypto.randomBytes(32).toString('hex')
  sessions.set(token, {
    profileId,
    discordId: discordUser.id,
    username:  discordUser.username || '',
    expiresAt: Date.now() + SESSION_TTL,
  })
  saveSessions()
  return { profileId, session: token }
}

// ── POST /auth/session ────────────────────────────────────────────────────────

router.post('/session', (req, res) => {
  const { discordUser } = req.body || {}
  if (!discordUser || !discordUser.id)
    return res.status(400).json({ error: 'Missing discordUser.id' })

  const result = createSession(discordUser)
  res.json(result)
})

// ── GET /api/servers/:key/sessions/:session ───────────────────────────────────

router.get('/:key/sessions/:session', async (req, res) => {
  if (!checkKey(req, res)) return

  pruneExpired()
  const entry = sessions.get(req.params.session)
  if (!entry)
    return res.status(404).json({ error: 'Session not found or expired.' })

  let access
  try {
    access = await serverAccess.getDiscordAccess(entry.discordId)
  } catch (err) {
    console.error('[master-api] access role check failed:', err.message)
    return res.status(503).json({ error: 'accessUnavailable' })
  }

  if (!access.allowed) {
    return res.status(403).json({ error: access.error || 'accessDenied' })
  }

  res.json({
    user: {
      id:        entry.profileId,
      discordId: entry.discordId,
      username:  entry.username,
      roles:     access.roles,
      permissions: factionWhitelist.getPlayerFactionPermissions(entry.discordId),
      gameFactions: factionWhitelist.getPlayerGameFactions(entry.discordId),
      factions: factionWhitelist.getPlayerAssignments(entry.discordId),
    },
  })
})

// ── GET /api/servers/:key/profiles/:profileId/check ──────────────────────────
// Used by the game server in offline mode to verify a profileId is allowed.

router.get('/:key/profiles/:profileId/check', async (req, res) => {
  if (!checkKey(req, res)) return

  const discordId = getProfileDiscordId(req, res)
  if (!discordId) return

  let access
  try {
    access = await serverAccess.getDiscordAccess(discordId)
  } catch (err) {
    console.error('[master-api] offline access role check failed:', err.message)
    return res.status(503).json({ error: 'accessUnavailable' })
  }

  if (!access.allowed) {
    return res.status(403).json({ error: access.error || 'accessDenied' })
  }

  res.json({
    allowed: true,
    roles: access.roles,
    ...getProfileFactionPayload(discordId),
  })
})

// â”€â”€ POST /api/servers/:key/profiles/:profileId/factions â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

router.post('/:key/profiles/:profileId/factions', (req, res) => {
  if (!checkKey(req, res) || !checkWriteToken(req, res)) return

  const discordId = getProfileDiscordId(req, res)
  if (!discordId) return

  try {
    const assignment = factionWhitelist.createAssignment({
      ...req.body,
      discordId,
    }, 'skymp-server')
    res.status(201).json({
      assignment,
      ...getProfileFactionPayload(discordId),
    })
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to assign faction' })
  }
})

// â”€â”€ DELETE /api/servers/:key/profiles/:profileId/factions/:assignmentId â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€

router.delete('/:key/profiles/:profileId/factions/:assignmentId', (req, res) => {
  if (!checkKey(req, res) || !checkWriteToken(req, res)) return

  const discordId = getProfileDiscordId(req, res)
  if (!discordId) return

  try {
    const belongsToPlayer = factionWhitelist
      .getPlayerAssignments(discordId)
      .some(assignment => assignment.id === req.params.assignmentId)
    if (!belongsToPlayer) return res.status(404).json({ error: 'assignment not found for player' })

    factionWhitelist.deleteAssignment(req.params.assignmentId)
    res.json({
      ok: true,
      ...getProfileFactionPayload(discordId),
    })
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to remove faction' })
  }
})

// ── GET /api/servers/:key/sessions/:session/balance ───────────────────────────

router.get('/:key/sessions/:session/balance', (req, res) => {
  if (!checkKey(req, res)) return

  pruneExpired()
  const entry = sessions.get(req.params.session)
  if (!entry)
    return res.status(404).json({ error: 'Session not found or expired.' })

  const balance = getBalance(entry.profileId)
  res.json({ user: { id: entry.profileId, balance } })
})

// ── POST /api/servers/:key/sessions/:session/purchase ────────────────────────

router.post('/:key/sessions/:session/purchase', (req, res) => {
  if (!checkKey(req, res)) return

  if (!checkWriteToken(req, res)) return

  pruneExpired()
  const entry = sessions.get(req.params.session)
  if (!entry)
    return res.status(404).json({ error: 'Session not found or expired.' })

  const { balanceToSpend } = req.body || {}
  if (typeof balanceToSpend !== 'number' || balanceToSpend < 0)
    return res.status(400).json({ error: 'balanceToSpend must be a non-negative number.' })

  const current = getBalance(entry.profileId)
  if (current < balanceToSpend)
    return res.json({ balanceSpent: 0, success: false })

  setBalance(entry.profileId, current - balanceToSpend)
  res.json({ balanceSpent: balanceToSpend, success: true })
})

// Fix for whitelist issues
async function isDiscordWhitelisted(discordId) {
  const serverAccess = require('../sources/serverAccess')
  const result = await serverAccess.getDiscordAccess(discordId)
  return result.allowed === true
}

module.exports = router
module.exports.lookupSession  = lookupSession
module.exports.createSession  = createSession
module.exports.isDiscordWhitelisted = isDiscordWhitelisted
