const router = require('express').Router()

/**
 * Update LATEST_VERSION here whenever you release a new launcher build.
 * Set DOWNLOAD_URL to the installer download link (e.g. a GitHub Releases URL).
 */
const LATEST_VERSION = '0.2.4'
const DOWNLOAD_URL   = 'https://cdn1.site-media.eu/images/0/26224023/SkyRPLauncherSetup0.2.4.exe-2aTNqgijg1wXTKMlUllUjg.zip'

router.get('/', (_req, res) => {
  res.json({
    version:     LATEST_VERSION,
    downloadUrl: DOWNLOAD_URL,
  })
})

module.exports = router
