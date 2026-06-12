const router = require('express').Router()
const fs     = require('fs')
const path   = require('path')

const MODLIST_PATH = path.join(__dirname, '..', 'data', 'modlist.json')

// allows editing data/modlist.json without restarting the backend.
router.get('/', (_req, res) => {
  try {
    res.json(JSON.parse(fs.readFileSync(MODLIST_PATH, 'utf8')))
  } catch (err) {
    res.status(500).json({ error: `modlist.json is missing or invalid: ${err.message}` })
  }
})

module.exports = router
