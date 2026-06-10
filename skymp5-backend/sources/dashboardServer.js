'use strict'

const express = require('express')
const path    = require('path')
const config  = require('../config')

function start() {
  if (!config.dashboardPort) return

  const app = express()
  const publicDir = path.join(__dirname, '..', 'public', 'dashboard')

  app.get('/dashboard-config.js', (_req, res) => {
    res.type('application/javascript').send(
      `window.SKYRP_DASHBOARD_CONFIG=${JSON.stringify({
        apiBaseUrl: config.dashboardApiBaseUrl,
        dashboardUrl: config.dashboardPublicUrl,
      })};`
    )
  })

  app.use(express.static(publicDir))
  app.get('*', (_req, res) => res.sendFile(path.join(publicDir, 'index.html')))

  app.listen(config.dashboardPort, () => {
    console.log(`SkyRP dashboard running on ${config.dashboardPublicUrl}`)
  })
}

module.exports = { start }
