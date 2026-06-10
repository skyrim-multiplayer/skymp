'use strict'

const { Router }        = require('express')
const requirePermission = require('../middleware/requirePermission')
const whitelist         = require('../sources/factionWhitelist')

const router = Router()

router.get('/', requirePermission('factions.view'), (_req, res) => {
  res.json(whitelist.list())
})

router.get('/players/:discordId', requirePermission('factions.view'), (req, res) => {
  res.json({
    discordId: req.params.discordId,
    permissions: whitelist.getPlayerFactionPermissions(req.params.discordId),
    gameFactions: whitelist.getPlayerGameFactions(req.params.discordId),
    assignments: whitelist.getPlayerAssignments(req.params.discordId),
  })
})

router.post('/assignments', requirePermission('factions.manage'), (req, res) => {
  try {
    const assignment = whitelist.createAssignment(req.body || {}, req.session.discordId)
    res.status(201).json(assignment)
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to create assignment' })
  }
})

router.put('/assignments/:id', requirePermission('factions.manage'), (req, res) => {
  try {
    res.json(whitelist.updateAssignment(req.params.id, req.body || {}, req.session.discordId))
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to update assignment' })
  }
})

router.delete('/assignments/:id', requirePermission('factions.manage'), (req, res) => {
  try {
    whitelist.deleteAssignment(req.params.id)
    res.json({ ok: true })
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to delete assignment' })
  }
})

module.exports = router
