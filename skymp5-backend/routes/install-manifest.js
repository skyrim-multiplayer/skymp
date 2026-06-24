const router = require('express').Router()
const fs     = require('fs')
const path   = require('path')

const MANIFEST_PATH = path.join(__dirname, '..', 'data', 'install-manifest.json')

// Compiled, hash-verified install manifest (built by scripts/compile-manifest.js).
// Served raw so the (potentially large, base64-inlined) file isn't re-stringified.
router.get('/', (_req, res) => {
  try {
    res.type('application/json').send(fs.readFileSync(MANIFEST_PATH, 'utf8'))
  } catch (err) {
    res.status(404).json({ error: `install-manifest.json not built yet: ${err.message}` })
  }
})

module.exports = router
