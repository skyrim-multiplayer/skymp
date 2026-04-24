'use strict'
/**
 * sign-gamemode.js
 *
 * Signs a built gamemode.js with the Ed25519 private key and appends the
 * signature as a comment so the SkyMP client can verify it.
 *
 * Usage (standalone):
 *   node sign-gamemode.js                   # signs gamemode.js in-place
 *   node sign-gamemode.js path/to/file.js   # signs a specific file in-place
 *
 * Also used as a module by the gamemode webpack config (runs automatically
 * after every build).
 *
 * Signature line format expected by the client (ServerJsVerificationService):
 *   // skymp:sig:y:<keyId>:<base64signature>
 */

const fs   = require('fs')
const path = require('path')
const { sign } = require('crypto')

const KEY_ID      = 'frostfall'
const PRIVATE_KEY = fs.readFileSync(path.join(__dirname, 'signing-private.pem'), 'utf8')
const SIG_PREFIX  = '// skymp:sig:y:'

/**
 * Signs the file at `filePath` in-place. Idempotent — strips any existing
 * signature before re-signing.
 * @param {string} filePath Absolute or relative path to the JS file to sign.
 */
function signFile(filePath) {
  let src = fs.readFileSync(filePath, 'utf8')

  // Strip any existing signature line so re-signing is idempotent.
  const lines = src.split('\n')
  if (lines[lines.length - 1].startsWith(SIG_PREFIX)) {
    lines.pop()
    src = lines.join('\n')
  }

  // Ensure the source ends with a newline before signing.
  if (!src.endsWith('\n')) src += '\n'

  const sig    = sign(null, Buffer.from(src, 'utf8'), PRIVATE_KEY).toString('base64')
  const signed = src + `${SIG_PREFIX}${KEY_ID}:${sig}\n`

  fs.writeFileSync(filePath, signed)
  console.log(`[sign-gamemode] Signed ${path.basename(filePath)} with key "${KEY_ID}"`)
}

/**
 * Signs an arbitrary JS string so the client's ServerJsVerificationService
 * can verify it.  Appends `\n// skymp:sig:y:<keyId>:<base64sig>` to the end.
 * The verifier reads everything before that separator newline as the payload,
 * so the signature covers exactly `src` as passed in.  Idempotent — strips
 * any existing signature before re-signing.
 *
 * @param {string} src  The JS source string to sign.
 * @returns {string}    The signed string (original content + signature line).
 */
function signScript(src) {
  // Strip any existing signature line so re-signing is idempotent.
  const idx = src.lastIndexOf('\n' + SIG_PREFIX)
  if (idx !== -1) src = src.substring(0, idx)

  const sig = sign(null, Buffer.from(src, 'utf8'), PRIVATE_KEY).toString('base64')
  return src + `\n${SIG_PREFIX}${KEY_ID}:${sig}`
}

module.exports = { signFile, signScript }

// Run as a standalone script when invoked directly.
if (require.main === module) {
  const target = process.argv[2]
    ? path.resolve(process.argv[2])
    : path.join(__dirname, 'gamemode.js')
  signFile(target)
}
