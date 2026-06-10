const fs = require('fs')
const path = require('path')
const dotenv = require('dotenv')

dotenv.config()

process.on('uncaughtException', (err) => {
  console.error('[uncaughtException] Server kept alive:', err.message)
})

process.on('unhandledRejection', (reason) => {
  console.error('[unhandledRejection] Server kept alive:', reason)
})

// Start WS relay alongside Express (independent port, see WS_PORT in .env)
require('./sources/wsRelay')

// Start Discord bot for role-based access checks
const discordBot = require('./sources/discordBot')
discordBot.start()

// Start the management dashboard on its own port/subdomain target.
const dashboardServer = require('./sources/dashboardServer')
dashboardServer.start()

const express  = require('express')
const cors     = require('cors')

const newsRoute        = require('./routes/news')
const statusRoute      = require('./routes/status')
const manifestRoute    = require('./routes/manifest')
const versionRoute     = require('./routes/version')
const serverinfoRoute  = require('./routes/serverinfo')
const metricsRoute     = require('./routes/metrics')
const masterApiRoute   = require('./routes/master-api')
const filesRoute       = require('./routes/files')
const modlistRoute     = require('./routes/modlist')
const serversRoute     = require('./routes/servers')
const webhookRoute     = require('./routes/webhook')
const adminRoute           = require('./routes/admin')
const dashAuthRoute        = require('./routes/dashboard-auth')
const skympCompatRoute     = require('./routes/skymp-compat')
const loreRoute            = require('./routes/lore')
const rulesRoute           = require('./routes/rules')
const whitelistRoute       = require('./routes/whitelist')
const whitelistNotesRoute  = require('./routes/whitelist-notes')
const factionWhitelistRoute = require('./routes/faction-whitelist')
const rolePermissionsRoute  = require('./routes/role-permissions')
const serverAccessRoute     = require('./routes/server-access')
const playersRoute          = require('./routes/players')

const app  = express()
const PORT = process.env.PORT || 4000

app.use(cors())

// Capture the raw request body so the webhook route can verify the
// GitHub HMAC-SHA256 signature without a separate body-parser step.
app.use(express.json({
  verify: (req, _res, buf) => { req.rawBody = buf },
}))

// Static file serving — root/ is installed into Skyrim/ (Data/ sub-dir)
app.use('/files/root', express.static(path.join(__dirname, 'public', 'files', 'root')))

// News images — served at /images/<filename>
app.use('/images', express.static(path.join(__dirname, 'public', 'images')))

app.use('/api/news',       newsRoute)
app.use('/api/status',     statusRoute)
app.use('/api/manifest',   manifestRoute)
app.use('/api/version',    versionRoute)
app.use('/api/serverinfo', serverinfoRoute)
app.use('/api/metrics',    metricsRoute)
app.use('/api/files',      filesRoute)
app.use('/api/modlist',    modlistRoute)
app.use('/api/servers',    serversRoute)
// SkyMP client Master-API compatibility — must be mounted before /api/servers
// so /api/users/login-discord/status is not swallowed by a shorter prefix.
app.use('/api/users',      skympCompatRoute)
app.use('/auth',           masterApiRoute)   // POST /auth/session
app.use('/api/servers',    masterApiRoute)   // GET  /api/servers/:key/sessions/:session
app.use('/webhooks',            webhookRoute)
app.use('/api/admin',           adminRoute)
app.use('/auth/dashboard',      dashAuthRoute)
app.use('/api/lore',            loreRoute)
app.use('/api/rules',           rulesRoute)
app.use('/api/whitelist',       whitelistRoute)
app.use('/api/whitelist-notes', whitelistNotesRoute)
app.use('/api/faction-whitelist', factionWhitelistRoute)
app.use('/api/role-permissions',  rolePermissionsRoute)
app.use('/api/server-access',      serverAccessRoute)
app.use('/api/players',            playersRoute)

app.listen(PORT, () => {
  console.log(`SkyRP backend running on http://localhost:${PORT}`)
})
