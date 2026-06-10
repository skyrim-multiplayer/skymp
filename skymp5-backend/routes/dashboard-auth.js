'use strict'
// ── Dashboard Discord OAuth ───────────────────────────────────────────────────
// Separate OAuth flow from the launcher — uses its own redirect_uri and issues
// dashboard session tokens instead of launcher session tokens.
//
// Required Discord app settings:
//   Redirects: add DISCORD_DASHBOARD_REDIRECT_URI (e.g. https://api.frostfall.online/auth/dashboard/callback)

const { Router }              = require('express')
const https                   = require('https')
const crypto                  = require('crypto')
const config                  = require('../config')
const sessions                = require('../sources/dashboardSessions')
const discordBot              = require('../sources/discordBot')
const { resolvePermissions, hasPermission } = require('../sources/permissions')

const router  = Router()

// state → { redirectUrl }  (10-min TTL)
const pending = new Map()

// ── GET /auth/dashboard/url ───────────────────────────────────────────────────
// Returns the Discord authorization URL. The website calls this then redirects
// the user's browser there.
// Query: ?redirect=<website-dashboard-url>  (where to return after auth)
router.get('/url', (req, res) => {
  if (!config.discordClientId) {
    return res.status(503).json({ error: 'Discord not configured on this server.' })
  }

  const state       = crypto.randomBytes(16).toString('hex')
  const redirectUrl = req.query.redirect || config.websiteUrl + '/dashboard'

  pending.set(state, { redirectUrl })
  setTimeout(() => pending.delete(state), 10 * 60 * 1000)

  const params = new URLSearchParams({
    client_id:     config.discordClientId,
    redirect_uri:  config.discordDashboardRedirectUri,
    response_type: 'code',
    scope:         'identify',
    state,
  })

  res.json({ url: `https://discord.com/api/oauth2/authorize?${params}` })
})

// ── GET /auth/dashboard/callback ──────────────────────────────────────────────
// Discord redirects here after the user authorizes (or cancels).
// On success: validate Discord ID → issue session → redirect to website with token.
// On failure: redirect to website dashboard with ?error=<reason>.
router.get('/callback', async (req, res) => {
  const { code, state, error } = req.query

  const fallbackRedirect = config.websiteUrl + '/dashboard'

  if (error) {
    return res.redirect(fallbackRedirect + '?error=cancelled')
  }

  if (!code || !state) {
    return res.status(400).send('Missing code or state.')
  }

  const pend = pending.get(state)
  if (!pend) {
    return res.redirect(fallbackRedirect + '?error=expired')
  }
  pending.delete(state)

  try {
    const tokenData   = await _tokenExchange(code)
    const user        = await _getUser(tokenData.access_token)

    const roleIds     = await discordBot.getMemberRoles(user.id)
    const permissions = resolvePermissions(roleIds)
    if (config.dashboardDiscordIds.includes(user.id) && !permissions.includes('admin.*')) {
      permissions.push('admin.*')
    }

    // Allow access through a configurable dashboard permission, or through the
    // legacy DASHBOARD_DISCORD_IDS allowlist.
    const isAllowed = hasPermission(permissions, 'dashboard.access') || config.dashboardDiscordIds.includes(user.id)
    if (!isAllowed) {
      return res.redirect(pend.redirectUrl + '?error=unauthorized')
    }

    const username = user.global_name || user.username
    const avatar   = user.avatar
      ? `https://cdn.discordapp.com/avatars/${user.id}/${user.avatar}.png?size=64`
      : null

    const token = sessions.create(user.id, username, avatar, roleIds, permissions)
    return res.redirect(`${pend.redirectUrl}?token=${token}`)

  } catch (err) {
    console.error('[dashboard-auth] callback error:', err.message)
    return res.redirect(pend.redirectUrl + '?error=server_error')
  }
})

// ── GET /auth/dashboard/me ────────────────────────────────────────────────────
// Validates a dashboard session token and returns the user's Discord info.
// Used by the website to confirm the session is still valid after page load.
router.get('/me', (req, res) => {
  const auth    = req.headers['authorization'] ?? ''
  const token   = auth.startsWith('Bearer ') ? auth.slice(7) : ''
  const session = sessions.validate(token)
  if (!session) return res.status(401).json({ error: 'invalid or expired session' })
  res.json({ ok: true, user: session })
})

// ── POST /auth/dashboard/logout ───────────────────────────────────────────────
router.post('/logout', (req, res) => {
  const auth  = req.headers['authorization'] ?? ''
  const token = auth.startsWith('Bearer ') ? auth.slice(7) : ''
  if (token) sessions.revoke(token)
  res.json({ ok: true })
})

// ── Discord helpers ───────────────────────────────────────────────────────────

function _tokenExchange(code) {
  return new Promise((resolve, reject) => {
    const body = new URLSearchParams({
      client_id:     config.discordClientId,
      client_secret: config.discordClientSecret,
      grant_type:    'authorization_code',
      code,
      redirect_uri:  config.discordDashboardRedirectUri,
    }).toString()

    const req = https.request({
      hostname: 'discord.com',
      path:     '/api/oauth2/token',
      method:   'POST',
      headers: {
        'Content-Type':   'application/x-www-form-urlencoded',
        'Content-Length': Buffer.byteLength(body),
      },
    }, res => {
      let data = ''
      res.on('data', c => { data += c })
      res.on('end', () => {
        const json = JSON.parse(data)
        if (json.error) reject(new Error(json.error_description || json.error))
        else resolve(json)
      })
    })
    req.on('error', reject)
    req.write(body)
    req.end()
  })
}

function _getUser(accessToken) {
  return new Promise((resolve, reject) => {
    const req = https.get({
      hostname: 'discord.com',
      path:     '/api/users/@me',
      headers:  { Authorization: `Bearer ${accessToken}` },
    }, res => {
      let data = ''
      res.on('data', c => { data += c })
      res.on('end', () => resolve(JSON.parse(data)))
    })
    req.on('error', reject)
  })
}

module.exports = router
