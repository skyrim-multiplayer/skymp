// ── WS Relay ───────────────────────────────────────────────────────────────────
//
// Single WebSocketServer that bridges two connection types:
//
//   gamemode  — one persistent connection from the SkyMP gamemode sandbox.
//               Identified by RELAY_SECRET on first message.
//
//   player    — one connection per in-game browser (skymp5-front).
//               Identified by a one-time nonce that the gamemode registers
//               before the browser connects.
//
// Message protocol (all JSON):
//
//   Handshake (first message, unauthenticated):
//     { type:'auth', role:'gamemode', secret:'...' }   → gamemode auth
//     { type:'auth', nonce:'...' }                     → player auth
//
//   Gamemode → relay:
//     { type:'register_nonce', nonce, userId }         → map nonce to userId
//     { type:'chat_deliver',   userId, msg }           → push msg to one player
//     { type:'chat_broadcast', msg }                   → push msg to all players
//
//   Player → relay → gamemode:
//     { type:'chat_send', text }                       → relayed with userId added
//
//   Relay → gamemode (informational):
//     { type:'player_connected',    userId }
//     { type:'player_disconnected', userId }

'use strict'

const { WebSocketServer, WebSocket } = require('ws')

const RELAY_SECRET = process.env.RELAY_SECRET || 'dev-relay-secret'
const WS_PORT      = parseInt(process.env.WS_PORT || '7778', 10)

// One gamemode socket (reconnects on crash/restart)
let gamemodeSocket = null

// userId → WebSocket (one per authenticated player browser)
const playerSockets = new Map()

// nonce → userId (registered by gamemode, consumed on player auth)
const nonceMap = new Map()

// ── Helpers ───────────────────────────────────────────────────────────────────

function send(ws, msg) {
  if (ws && ws.readyState === WebSocket.OPEN) {
    ws.send(JSON.stringify(msg))
  }
}

function toGamemode(msg) {
  send(gamemodeSocket, msg)
}

// ── Server ────────────────────────────────────────────────────────────────────

const wss = new WebSocketServer({ port: WS_PORT })

wss.on('connection', (ws) => {
  let role   = null   // 'gamemode' | 'player'
  let userId = null

  ws.on('message', (raw) => {
    let msg
    try { msg = JSON.parse(raw.toString()) } catch { return }

    // ── Auth handshake (must be first message) ────────────────────────────────
    if (role === null) {
      if (msg.type === 'auth' && msg.role === 'gamemode') {
        if (msg.secret !== RELAY_SECRET) {
          ws.close(4001, 'bad secret')
          return
        }
        role = 'gamemode'
        gamemodeSocket = ws
        send(ws, { type: 'auth_ok', role: 'gamemode' })
        console.log('[ws-relay] gamemode authenticated')
        return
      }

      if (msg.type === 'auth' && msg.nonce) {
        const uid = nonceMap.get(msg.nonce)
        if (uid === undefined) {
          send(ws, { type: 'auth_fail', reason: 'unknown_nonce' })
          ws.close(4002, 'unknown nonce')
          return
        }
        role   = 'player'
        userId = uid
        nonceMap.delete(msg.nonce)
        playerSockets.set(userId, ws)
        send(ws, { type: 'auth_ok', role: 'player', userId })
        toGamemode({ type: 'player_connected', userId })
        console.log(`[ws-relay] player ${userId} authenticated`)
        return
      }

      // Unknown or missing auth — reject immediately
      ws.close(4000, 'auth required')
      return
    }

    // ── Gamemode messages ─────────────────────────────────────────────────────
    if (role === 'gamemode') {
      if (msg.type === 'register_nonce') {
        nonceMap.set(msg.nonce, msg.userId)
        return
      }

      if (msg.type === 'chat_deliver') {
        const sock = playerSockets.get(msg.userId)
        send(sock, { type: 'chat_msg', msg: msg.msg })
        return
      }

      if (msg.type === 'chat_broadcast') {
        const payload = JSON.stringify({ type: 'chat_msg', msg: msg.msg })
        for (const sock of playerSockets.values()) {
          if (sock.readyState === WebSocket.OPEN) sock.send(payload)
        }
        return
      }

      return
    }

    // ── Player messages ───────────────────────────────────────────────────────
    if (role === 'player') {
      if (msg.type === 'chat_send' && typeof msg.text === 'string') {
        toGamemode({ type: 'chat_send', userId, text: msg.text })
      }
      return
    }
  })

  ws.on('close', () => {
    if (role === 'gamemode') {
      if (gamemodeSocket === ws) gamemodeSocket = null
      console.log('[ws-relay] gamemode disconnected')
      return
    }
    if (role === 'player') {
      playerSockets.delete(userId)
      toGamemode({ type: 'player_disconnected', userId })
      console.log(`[ws-relay] player ${userId} disconnected`)
    }
  })

  ws.on('error', (err) => {
    console.error(`[ws-relay] socket error (${role ?? 'unauthenticated'}):`, err.message)
  })
})

wss.on('error', (err) => {
  console.error('[ws-relay] server error:', err.message)
})

console.log(`[ws-relay] listening on ws://0.0.0.0:${WS_PORT}`)

module.exports = wss
