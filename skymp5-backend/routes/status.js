const router = require('express').Router()
const net    = require('net')
const http   = require('http')
const config = require('../config')

// UDP reachability check for the game port
// TCP wasnt working lul
function udpCheck(host, port) {
  return new Promise(resolve => {
    const dgram = require('dgram')
    const socket = dgram.createSocket('udp4')
    const msg = Buffer.from('ping')
    let resolved = false

    const done = (result) => {
      if (resolved) return
      resolved = true
      try { socket.close() } catch {}
      resolve(result)
    }

    socket.on('error', () => done(false))

    socket.send(msg, 0, msg.length, port, host, (err) => {
      if (err) return done(false)
      done(true)
    })

    setTimeout(() => done(false), 3000)
  })
}

// Fetch Prometheus metrics from SkyMP HTTP UI and derive online player count.
// Online players ≈ skymp_connects_total − skymp_disconnects_total
function fetchPlayerCount(host, uiPort) {
  return new Promise(resolve => {
    const req = http.get(
      { hostname: host, port: uiPort, path: '/metrics', timeout: 3000 },
      res => {
        let raw = ''
        res.on('data', c => { raw += c })
        res.on('end', () => {
          const val = name => {
            const m = raw.match(new RegExp(`^${name}\\s+(\\d+)`, 'm'))
            return m ? parseInt(m[1], 10) : null
          }
          const connects    = val('skymp_connects_total')
          const disconnects = val('skymp_disconnects_total')
          if (connects !== null && disconnects !== null) {
            resolve(Math.max(0, connects - disconnects))
          } else {
            resolve(null)
          }
        })
      }
    )
    req.on('error',   () => resolve(null))
    req.on('timeout', () => { req.destroy(); resolve(null) })
  })
}

router.get('/', async (_req, res) => {
  const { skyrimServerHost: host, skyrimServerPort: gamePort, skympUiPort: uiPort } = config
  const online  = await udpCheck(host, gamePort)
  const players = online ? await fetchPlayerCount(host, uiPort) : null
  res.json({ status: online ? 'online' : 'offline', players })
})

module.exports = router
