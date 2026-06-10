'use strict'
// ── Admin proxy ───────────────────────────────────────────────────────────────
// Validates the bearer token, then forwards requests to the Frostfall-Admin
// service. Keeps the admin service off the public internet — only the backend
// is exposed; the admin process binds to localhost only.

const { Router } = require('express')
const http       = require('http')
const https      = require('https')
const crypto     = require('crypto')
const config     = require('../config')
const sessions   = require('../sources/dashboardSessions')

const router = Router()

function validateToken(req, res) {
  if (!config.adminToken) {
    res.status(503).json({ error: 'admin service not configured (ADMIN_TOKEN not set)' })
    return false
  }
  const auth     = req.headers['authorization'] ?? ''
  const provided = auth.startsWith('Bearer ') ? auth.slice(7) : ''
  if (!provided) {
    res.status(401).json({ error: 'missing authorization header' })
    return false
  }

  // Accept a valid dashboard session token (Discord-authenticated admin)
  if (sessions.validate(provided)) return true

  // Fall back to static ADMIN_TOKEN
  const expected = Buffer.from(config.adminToken)
  const actual   = Buffer.from(provided)
  if (actual.length !== expected.length || !crypto.timingSafeEqual(actual, expected)) {
    res.status(401).json({ error: 'invalid token' })
    return false
  }
  return true
}

// Forward any request under /api/admin/* to the admin service
router.all('/*', (req, res) => {
  if (!validateToken(req, res)) return

  const base     = new URL(config.adminUrl)
  const useHttps = base.protocol === 'https:'
  const lib      = useHttps ? https : http

  // Strip the /api/admin prefix — forward the remainder to the admin service
  const adminPath = '/api' + req.path  // e.g. /api/admin/server/start → /api/server/start

  const options = {
    hostname: base.hostname,
    port:     base.port || (useHttps ? 443 : 80),
    path:     adminPath + (req.url.includes('?') ? '?' + req.url.split('?')[1] : ''),
    method:   req.method,
    headers: {
      'Content-Type':  'application/json',
      'Authorization': `Bearer ${config.adminToken}`,
    },
  }

  const proxyReq = lib.request(options, proxyRes => {
    res.status(proxyRes.statusCode)
    let body = ''
    proxyRes.on('data', chunk => { body += chunk })
    proxyRes.on('end', () => {
      try { res.json(JSON.parse(body)) }
      catch { res.send(body) }
    })
  })

  proxyReq.on('error', err => {
    res.status(502).json({ error: 'admin service unreachable', detail: err.message })
  })

  if (req.body && Object.keys(req.body).length > 0) {
    proxyReq.write(JSON.stringify(req.body))
  }

  proxyReq.end()
})

module.exports = router
