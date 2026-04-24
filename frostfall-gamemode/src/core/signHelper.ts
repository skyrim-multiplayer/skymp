// ── Script signing helper ─────────────────────────────────────────────────────
//
// Signs a JS string so the SkyMP client's ServerJsVerificationService can
// verify it against the public key advertised in the Frostfall serverinfo.
//
// sign-gamemode.js lives one level above the gamemode directory.  We load it
// at RUNTIME via a dynamic path (eval'd require) so webpack cannot statically
// analyse the dependency and attempt to bundle it — the file must remain on
// disk so it can read signing-private.pem relative to its own __dirname.
//
// If the file is not present (e.g. dev environment without the key), signScript
// returns the src unchanged and the client falls back to skipping verification
// when no publicKeys are configured.

import path from 'path'

let _sign: ((src: string) => string) | null | undefined = undefined

function loadSigner(): ((src: string) => string) | null {
  try {
    // eval() prevents webpack from resolving the require at bundle time.
    // eslint-disable-next-line no-eval
    const mod = eval('require')(path.join(process.cwd(), 'sign-gamemode.js'))
    if (typeof mod?.signScript === 'function') {
      console.log('[signHelper] Loaded sign-gamemode.js — scripts will be signed')
      return mod.signScript as (src: string) => string
    }
    console.warn('[signHelper] sign-gamemode.js has no signScript export — scripts will be unsigned')
  } catch (err: any) {
    console.warn('[signHelper] Could not load sign-gamemode.js:', err?.message ?? err,
      '— scripts will be unsigned')
  }
  return null
}

/**
 * Signs a JS source string so the client can verify it.
 * Appends `\n// skymp:sig:y:<keyId>:<sig>` to the content.
 * Returns the original string unchanged if the signer is unavailable.
 */
export function signScript(src: string): string {
  if (_sign === undefined) _sign = loadSigner()
  return _sign ? _sign(src) : src
}
