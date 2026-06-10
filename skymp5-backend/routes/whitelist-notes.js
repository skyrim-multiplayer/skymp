'use strict'
// ── Whitelist staff notes API ─────────────────────────────────────────────────
// A single editable document with notes for staff handling whitelist applications.
// Read requires 'staff.whitelist_info', write requires 'rules.write' (Management).

const { Router }        = require('express')
const fs                = require('fs')
const path              = require('path')
const requirePermission = require('../middleware/requirePermission')

const router = Router()
const FILE   = path.join(__dirname, '..', 'data', 'whitelist-staff-notes.json')

function load() {
  try {
    return JSON.parse(fs.readFileSync(FILE, 'utf8'))
  } catch {
    return { content: '', updatedAt: null, updatedBy: null }
  }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2))
}

// GET /api/whitelist-notes  — requires staff.whitelist_info
router.get('/', requirePermission('staff.whitelist_info'), (_req, res) => {
  res.json(load())
})

// PUT /api/whitelist-notes  — requires rules.write
router.put('/', requirePermission('rules.write'), (req, res) => {
  const { content } = req.body || {}
  if (content === undefined) return res.status(400).json({ error: 'content required' })
  const doc = {
    content,
    updatedAt: new Date().toISOString(),
    updatedBy: req.session.discordId,
  }
  save(doc)
  res.json(doc)
})

module.exports = router
