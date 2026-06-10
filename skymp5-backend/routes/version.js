const router = require('express').Router()

/**
 * Update LATEST_VERSION here whenever you release a new launcher build.
 * Set DOWNLOAD_URL to the installer download link (e.g. a GitHub Releases URL).
 */
const LATEST_VERSION = '0.1.1'
const DOWNLOAD_URL   = 'https://skyrimroleplay.co.uk/download'

router.get('/', (_req, res) => {
  res.json({
    version:     LATEST_VERSION,
    downloadUrl: DOWNLOAD_URL,
  })
})

module.exports = router
