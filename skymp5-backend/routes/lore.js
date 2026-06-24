'use strict'
// ── Lore API ──────────────────────────────────────────────────────────────────
// CRUD for lore wiki entries stored in data/lore.json.
// Public read (GET). Write operations require 'lore.write' permission.

const { Router }        = require('express')
const crypto            = require('crypto')
const fs                = require('fs')
const path              = require('path')
const requirePermission = require('../middleware/requirePermission')

const router = Router()
const FILE   = path.join(__dirname, '..', 'data', 'lore.json')

function load() {
  try { return JSON.parse(fs.readFileSync(FILE, 'utf8')) } catch { return [] }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2))
}

// GET /api/lore  — public; optional ?category= filter
router.get('/', (req, res) => {
  const entries  = load()
  const category = req.query.category
  res.json(category ? entries.filter(e => e.category === category) : entries)
})

// PUT /api/lore/reorder  — requires lore.write  (must be before /:id)
router.put('/reorder', requirePermission('lore.write'), (req, res) => {
  const { ids } = req.body || {}
  if (!Array.isArray(ids)) return res.status(400).json({ error: 'ids array required' })
  const entries  = load()
  const map      = new Map(entries.map(e => [e.id, e]))
  const reordered = ids.map(id => map.get(id)).filter(Boolean)
  const included  = new Set(ids)
  for (const e of entries) if (!included.has(e.id)) reordered.push(e)
  save(reordered)
  res.json({ ok: true })
})

// GET /api/lore/:id  — public
router.get('/:id', (req, res) => {
  const entry = load().find(e => e.id === req.params.id)
  if (!entry) return res.status(404).json({ error: 'not found' })
  res.json(entry)
})

// POST /api/lore  — requires lore.write
router.post('/', requirePermission('lore.write'), (req, res) => {
  const { title, category, content } = req.body || {}
  if (!title || content === undefined) {
    return res.status(400).json({ error: 'title and content are required' })
  }
  const entries = load()
  const now     = new Date().toISOString()
  const entry   = {
    id:        crypto.randomUUID(),
    title,
    category:  category || 'general',
    content,
    createdAt: now,
    updatedAt: now,
    createdBy: req.session.discordId,
  }
  entries.push(entry)
  save(entries)
  res.status(201).json(entry)
})

// PUT /api/lore/:id  — requires lore.write
router.put('/:id', requirePermission('lore.write'), (req, res) => {
  const entries = load()
  const idx     = entries.findIndex(e => e.id === req.params.id)
  if (idx === -1) return res.status(404).json({ error: 'not found' })

  const { title, category, content } = req.body || {}
  const entry = entries[idx]
  if (title    !== undefined) entry.title    = title
  if (category !== undefined) entry.category = category
  if (content  !== undefined) entry.content  = content
  entry.updatedAt = new Date().toISOString()

  save(entries)
  res.json(entry)
})

// DELETE /api/lore/:id  — requires lore.write
router.delete('/:id', requirePermission('lore.write'), (req, res) => {
  const entries = load()
  const idx     = entries.findIndex(e => e.id === req.params.id)
  if (idx === -1) return res.status(404).json({ error: 'not found' })
  entries.splice(idx, 1)
  save(entries)
  res.json({ ok: true })
})

module.exports = router
