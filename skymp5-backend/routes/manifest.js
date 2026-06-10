const router = require('express').Router()
const path   = require('path')
const fs     = require('fs')
const crypto = require('crypto')

const FILES_ROOT = path.join(__dirname, '..', 'public', 'files')

// root/ → installed into {skyrimPath}/   (Data/ sub-dir)
const BUCKETS = [
  { dir: path.join(FILES_ROOT, 'root'), urlBase: '/files/root', destBase: '' },
]

function sha256(filePath) {
  try {
    return crypto.createHash('sha256').update(fs.readFileSync(filePath)).digest('hex')
  } catch {
    return null
  }
}

function walkBucket(dir, urlBase, destBase) {
  const entries = []

  function recurse(absDir, rel) {
    let names
    try { names = fs.readdirSync(absDir) } catch { return }
    for (const name of names) {
      const abs    = path.join(absDir, name)
      const relNew = rel ? `${rel}/${name}` : name
      let stat
      try { stat = fs.statSync(abs) } catch { continue }
      if (stat.isDirectory()) {
        recurse(abs, relNew)
      } else {
        entries.push({
          url:    `${urlBase}/${relNew}`,
          dest:   destBase ? `${destBase}/${relNew}` : relNew,
          sha256: sha256(abs),
          size:   stat.size,
        })
      }
    }
  }

  recurse(dir, '')
  return entries
}

router.get('/', (_req, res) => {
  const manifest = BUCKETS.flatMap(b => walkBucket(b.dir, b.urlBase, b.destBase))

  if (manifest.length === 0) {
    return res.status(503).json({
      error: 'No files found. Run "npm run populate" to copy client files into backend/public/files/.'
    })
  }

  res.json(manifest)
})

module.exports = router
