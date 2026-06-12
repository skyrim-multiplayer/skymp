const router = require('express').Router()

/**
 * Update LATEST_VERSION here whenever you release a new launcher build.
 * Set DOWNLOAD_URL to the installer download link (e.g. a GitHub Releases URL).
 */
const LATEST_VERSION = '0.3.0'
const DOWNLOAD_URL   = 'https://cdn1.site-media.eu/images/0/26240069/SkyRPLauncherSetup0.3.0.exe-3Fw9RLl1-jGYq0xPdomnJg.zip'

router.get('/', (_req, res) => {
  res.json({
    version:     LATEST_VERSION,
    downloadUrl: DOWNLOAD_URL,
  })
})

module.exports = router
