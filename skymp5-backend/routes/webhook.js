'use strict'

/**
 * GitHub webhook handler
 * Route: POST /webhooks/github
 *
 * GitHub delivers a signed JSON payload whenever a push is made to the
 * Frostfall-Client repository.  This handler:
 *   1. Verifies the HMAC-SHA256 signature (X-Hub-Signature-256 header).
 *   2. Responds 200 immediately so GitHub doesn't time out.
 *   3. Runs `git pull` in sources/client/ then rebuilds public/files/root/.
 *
 * Environment variables
 * ─────────────────────
 *   GITHUB_WEBHOOK_SECRET  (required)
 *     The same secret you enter in the GitHub repo → Settings → Webhooks form.
 *
 * GitHub webhook setup (Frostfall-Client repo)
 * ─────────────────────────────────────────────
 *   Payload URL : https://<your-server>/webhooks/github
 *   Content type: application/json
 *   Secret      : value of GITHUB_WEBHOOK_SECRET
 *   Events      : Just the push event
 */

const crypto  = require('crypto')
const path    = require('path')
const { execFile } = require('child_process')
const express = require('express')
const rateLimit = require('express-rate-limit')
const { mergeSourcesIntoRoot } = require('../scripts/merge-files')

const router = express.Router()

const githubWebhookLimiter = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100,
  standardHeaders: true,
  legacyHeaders: false
})

const CLIENT_DIR   = path.join(__dirname, '..', 'sources', 'client')
const DEFAULT_BRANCH = process.env.CLIENT_BRANCH || 'refs/heads/main'

// ── Signature verification ────────────────────────────────────────────────────

/**
 * Constant-time comparison of expected vs received HMAC-SHA256 signature.
 * Returns true if the payload was signed with `secret`.
 */
function verifySignature(secret, rawBody, receivedSig) {
  if (typeof receivedSig !== 'string') return false

  const expected = 'sha256=' + crypto
    .createHmac('sha256', secret)
    .update(rawBody)
    .digest('hex')

  try {
    // Buffers must be the same length for timingSafeEqual
    if (expected.length !== receivedSig.length) return false
    return crypto.timingSafeEqual(
      Buffer.from(expected),
      Buffer.from(receivedSig)
    )
  } catch {
    return false
  }
}

// ── Update pipeline ───────────────────────────────────────────────────────────

/**
 * Pull latest from the client repo and rebuild the file root.
 * Runs asynchronously after the HTTP response is already sent.
 */
function pullAndMerge() {
  console.log('[webhook] Running git pull in', CLIENT_DIR)

  execFile('git', ['-C', CLIENT_DIR, 'pull', '--ff-only'], (err, stdout, stderr) => {
    if (err) {
      console.error('[webhook] git pull failed:', stderr.trim() || err.message)
      return
    }

    const summary = stdout.trim()
    if (summary === 'Already up to date.') {
      console.log('[webhook] Client already up to date — skipping merge.')
      return
    }

    console.log('[webhook] git pull:', summary)

    mergeSourcesIntoRoot().catch(mergeErr => {
      console.error('[webhook] merge failed:', mergeErr.message)
    })
  })
}

// ── Route ─────────────────────────────────────────────────────────────────────

router.post('/github', githubWebhookLimiter, (req, res) => {
  const secret = process.env.GITHUB_WEBHOOK_SECRET

  // Secret must be configured — reject silently otherwise
  if (!secret) {
    console.error('[webhook] GITHUB_WEBHOOK_SECRET is not set.')
    return res.status(500).json({ error: 'Webhook not configured on server.' })
  }

  // Signature verification
  const sig = req.headers['x-hub-signature-256']
  if (!verifySignature(secret, req.rawBody, sig)) {
    console.warn('[webhook] Rejected request with invalid signature.')
    return res.status(401).json({ error: 'Invalid signature.' })
  }

  const event = req.headers['x-github-event']
  const body  = req.body  // already parsed by express.json()

  // Acknowledge immediately — GitHub times out after 10 s
  res.json({ ok: true, event })

  // ── Handle push to the tracked branch ──────────────────────────────────────
  if (event === 'push') {
    const ref = body?.ref
    if (ref !== DEFAULT_BRANCH) {
      console.log(`[webhook] Push to ${ref} — not the tracked branch (${DEFAULT_BRANCH}), ignoring.`)
      return
    }

    const pusher  = body?.pusher?.name || 'unknown'
    const commits = body?.commits?.length ?? 0
    console.log(`[webhook] Push by ${pusher}: ${commits} commit(s) on ${ref}`)

    pullAndMerge()
    return
  }

  // ── ping — GitHub sends this when the webhook is first created ─────────────
  if (event === 'ping') {
    console.log('[webhook] Ping received — webhook is connected.')
    return
  }

  console.log(`[webhook] Unhandled event: ${event}`)
})

module.exports = router
