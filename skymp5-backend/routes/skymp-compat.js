'use strict'

/**
 * SkyMP Master API compatibility routes.
 *
 * Implements the three endpoints that the SkyMP game client (authService.ts)
 * expects on the master server, bridging them to the existing session
 * infrastructure in master-api.js.
 *
 * Mounted in server.js as:
 *   app.use('/api/users', skympCompatRoute)
 *
 * Endpoints:
 *
 *   GET /api/users/login-discord?state=<hex>
 *     Client generates its own state token and opens this URL in the system
 *     browser.  We register the state as pending and redirect to Discord OAuth.
 *
 *   GET /api/users/login-discord/callback?code=...&state=...
 *     Discord's registered redirect URI.  Exchanges the code, creates a
 *     session, and marks the state as done so the polling endpoint can return
 *     the result.
 *
 *   GET /api/users/login-discord/status?state=<hex>
 *     Client polls this while waiting for the browser OAuth to finish.
 *     401 — still pending
 *     200 — done; returns { token, masterApiId, discordUsername, discordDiscriminator, discordAvatar }
 *           The `token` is the play-session token; /me/play just validates it.
 *     403 — unknown or expired state
 *
 *   POST /api/users/me/play/:serverKey
 *     Headers: { authorization: <token> }
 *     Body:    {} (ignored)
 *     Validates the token is a live session and returns { session: token }.
 */

const router  = require('express').Router()
const https   = require('https')
const crypto  = require('crypto')
const config  = require('../config')

// ── Pending/completed auth store ─────────────────────────────────────────────
// state → { status: 'pending'|'done', expiresAt, ...sessionFields }
// Pending entries expire after 10 minutes (OAuth timeout).
// Done entries expire after 5 minutes (client has time to call /status once).

const authStates = new Map()
const PENDING_TTL  = 10 * 60 * 1000
const DONE_TTL     =  5 * 60 * 1000

function pruneAuthStates() {
  const now = Date.now()
  for (const [k, v] of authStates)
    if (v.expiresAt < now) authStates.delete(k)
}

// ── GET /api/users/login-discord ─────────────────────────────────────────────

router.get('/login-discord', (req, res) => {
  const { state } = req.query
  if (!state) return res.status(400).send('Missing state parameter.')

  if (!config.discordClientId) {
    return res.status(503).send('Discord OAuth is not configured on this server.')
  }

  // Register the state so we can distinguish "unknown" from "pending" in /status.
  authStates.set(state, { status: 'pending', expiresAt: Date.now() + PENDING_TTL })

  const params = new URLSearchParams({
    client_id:     config.discordClientId,
    redirect_uri:  config.discordRedirectUri,
    response_type: 'code',
    scope:         'identify',
    state,
  })

  res.redirect(`https://discord.com/api/oauth2/authorize?${params}`)
})

// ── GET /api/users/login-discord/callback ────────────────────────────────────
// Discord's registered redirect URI.  Must be set to:
//   <MASTER_URL>/api/users/login-discord/callback
// in both DISCORD_REDIRECT_URI (.env) and the Discord application settings.

router.get('/login-discord/callback', async (req, res) => {
  const { code, state, error } = req.query

  if (error) {
    if (state && authStates.has(state)) authStates.delete(state)
    return res.status(400).send(`Discord OAuth error: ${error}`)
  }

  if (!code || !state) {
    return res.status(400).send('Missing code or state.')
  }

  const entry = authStates.get(state)
  if (!entry || entry.status !== 'pending') {
    return res.status(400).send('Unknown or expired OAuth state.')
  }

  try {
    const tokenData = await discordTokenExchange({
      client_id:     config.discordClientId,
      client_secret: config.discordClientSecret,
      grant_type:    'authorization_code',
      code,
      redirect_uri:  config.discordRedirectUri,
    })

    const user = await discordGetUser(tokenData.access_token)

    const { createSession } = require('./master-api')
    const { session, profileId } = createSession({
      id:       user.id,
      username: user.global_name || user.username,
    })

    authStates.set(state, {
      status:              'done',
      expiresAt:           Date.now() + DONE_TTL,
      session,
      profileId,
      discordId:           user.id,
      username:            user.global_name || user.username,
      avatar: user.avatar
        ? `https://cdn.discordapp.com/avatars/${user.id}/${user.avatar}.png?size=64`
        : null,
    })

    res.send('<html><body><h2>Authorised. You may close this tab and return to the game.</h2></body></html>')
  } catch (err) {
    console.error('[skymp-compat] Discord callback error:', err.message)
    authStates.delete(state)
    res.status(500).send('Authentication failed.')
  }
})

// ── GET /api/users/login-discord/status ──────────────────────────────────────

router.get('/login-discord/status', (req, res) => {
  const { state } = req.query
  if (!state) return res.status(400).json({ error: 'Missing state.' })

  pruneAuthStates()

  const entry = authStates.get(state)

  if (!entry)          return res.status(403).json({ error: 'Unknown or expired state.' })
  if (entry.status === 'pending') return res.status(401).json({ error: 'Auth not completed yet.' })

  // Consume on first successful read.
  authStates.delete(state)

  res.json({
    token:                entry.session,
    masterApiId:          entry.profileId,
    discordUsername:      entry.username  || null,
    discordDiscriminator: null,            // not stored; nullable in SkyMP
    discordAvatar:        entry.avatar    || null,
  })
})

// ── POST /api/users/me/play/:serverKey ───────────────────────────────────────

router.post('/me/play/:serverKey', (req, res) => {
  const token = req.headers['authorization']
  if (!token) return res.status(401).json({ error: 'Missing authorization header.' })

  if (req.params.serverKey !== config.serverMasterKey) {
    return res.status(403).json({ error: 'Invalid server key.' })
  }

  const { lookupSession } = require('./master-api')
  const session = lookupSession(token)
  if (!session) return res.status(401).json({ error: 'Invalid or expired session token.' })

  res.json({ session: token })
})

// ── Discord API helpers ───────────────────────────────────────────────────────

function discordTokenExchange(params) {
  return new Promise((resolve, reject) => {
    const body = new URLSearchParams(params).toString()
    const req  = https.request(
      {
        hostname: 'discord.com',
        path:     '/api/oauth2/token',
        method:   'POST',
        headers:  {
          'Content-Type':   'application/x-www-form-urlencoded',
          'Content-Length': Buffer.byteLength(body),
        },
      },
      res => {
        let data = ''
        res.on('data', c => { data += c })
        res.on('end', () => {
          const json = JSON.parse(data)
          if (json.error) reject(new Error(json.error_description || json.error))
          else resolve(json)
        })
      }
    )
    req.on('error', reject)
    req.write(body)
    req.end()
  })
}

function discordGetUser(accessToken) {
  return new Promise((resolve, reject) => {
    const req = https.get(
      {
        hostname: 'discord.com',
        path:     '/api/users/@me',
        headers:  { Authorization: `Bearer ${accessToken}` },
      },
      res => {
        let data = ''
        res.on('data', c => { data += c })
        res.on('end', () => resolve(JSON.parse(data)))
      }
    )
    req.on('error', reject)
  })
}

module.exports = router
