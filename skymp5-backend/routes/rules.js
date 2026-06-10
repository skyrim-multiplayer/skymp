'use strict'
// ── Rules API ─────────────────────────────────────────────────────────────────
// CRUD for RP rules stored as a flat array in data/rules.json.
// Public read (GET). Write operations require 'rules.write' permission (Management only).

const { Router }        = require('express')
const crypto            = require('crypto')
const fs                = require('fs')
const path              = require('path')
const requirePermission = require('../middleware/requirePermission')

const router = Router()
const FILE   = path.join(__dirname, '..', 'data', 'rules.json')

function load() {
  try { return JSON.parse(fs.readFileSync(FILE, 'utf8')) } catch { return [] }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2))
}

// GET /api/rules  — public
router.get('/', (_req, res) => {
  res.json(load())
})

// POST /api/rules  — requires rules.write
router.post('/', requirePermission('rules.write'), (req, res) => {
  const { title, content } = req.body || {}
  if (!title || content === undefined) {
    return res.status(400).json({ error: 'title and content are required' })
  }
  const rules = load()
  const rule  = {
    id:        crypto.randomUUID(),
    order:     rules.length + 1,
    title,
    content,
    updatedAt: new Date().toISOString(),
    updatedBy: req.session.discordId,
  }
  rules.push(rule)
  save(rules)
  res.status(201).json(rule)
})

// PUT /api/rules/reorder  — requires rules.write  (must be before /:id)
router.put('/reorder', requirePermission('rules.write'), (req, res) => {
  const { ids } = req.body || {}
  if (!Array.isArray(ids)) return res.status(400).json({ error: 'ids array required' })
  const rules    = load()
  const map      = new Map(rules.map(r => [r.id, r]))
  const reordered = ids.map(id => map.get(id)).filter(Boolean)
  const included  = new Set(ids)
  for (const r of rules) if (!included.has(r.id)) reordered.push(r)
  reordered.forEach((r, i) => { r.order = i + 1 })
  save(reordered)
  res.json({ ok: true })
})

// PUT /api/rules/:id  — requires rules.write
router.put('/:id', requirePermission('rules.write'), (req, res) => {
  const rules = load()
  const idx   = rules.findIndex(r => r.id === req.params.id)
  if (idx === -1) return res.status(404).json({ error: 'not found' })

  const { title, content } = req.body || {}
  const rule = rules[idx]
  if (title   !== undefined) rule.title   = title
  if (content !== undefined) rule.content = content
  rule.updatedAt = new Date().toISOString()
  rule.updatedBy = req.session.discordId

  save(rules)
  res.json(rule)
})

// DELETE /api/rules/:id  — requires rules.write
router.delete('/:id', requirePermission('rules.write'), (req, res) => {
  const rules = load()
  const idx   = rules.findIndex(r => r.id === req.params.id)
  if (idx === -1) return res.status(404).json({ error: 'not found' })
  rules.splice(idx, 1)
  // Re-number remaining rules
  rules.forEach((r, i) => { r.order = i + 1 })
  save(rules)
  res.json({ ok: true })
})

module.exports = router
