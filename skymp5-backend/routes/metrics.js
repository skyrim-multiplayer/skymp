const router = require('express').Router()
const http   = require('http')
const config = require('../config')

function metricsAuthHeader() {
  const { metricsUser: user, metricsPassword: password } = config
  if (user && password) {
    return { Authorization: `Basic ${Buffer.from(`${user}:${password}`).toString('base64')}` }
  }
  return {}
}

function fetchRaw(host, port) {
  return new Promise((resolve, reject) => {
    const req = http.get(
      {
        hostname: host,
        port,
        path:     '/metrics',
        timeout:  5000,
        headers:  metricsAuthHeader(),
      },
      res => {
        if (res.statusCode < 200 || res.statusCode >= 300) {
          res.resume()
          reject(new Error(`HTTP ${res.statusCode}`))
          return
        }
        let raw = ''
        res.on('data', c => { raw += c })
        res.on('end', () => resolve(raw))
      }
    )
    req.on('error',   reject)
    req.on('timeout', () => { req.destroy(); reject(new Error('timeout')) })
  })
}

function parsePrometheus(raw) {
  const result = {}
  for (const line of raw.split('\n')) {
    if (line.startsWith('#') || !line.trim()) continue
    const m = line.match(/^(skymp_\S+)\s+([\d.e+\-]+)/)
    if (m) result[m[1]] = parseFloat(m[2])
  }
  return result
}

router.get('/', async (_req, res) => {
  const { skyrimServerHost: host, skympUiPort: port } = config
  try {
    const raw     = await fetchRaw(host, port)
    const metrics = parsePrometheus(raw)
    res.json({ ok: true, metrics })
  } catch (err) {
    res.json({ ok: false, error: err.message })
  }
})

module.exports = router
