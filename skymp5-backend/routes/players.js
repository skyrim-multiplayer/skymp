'use strict'

const { Router }        = require('express')
const requirePermission = require('../middleware/requirePermission')
const profiles          = require('../sources/profiles')
const players           = require('../sources/players')
const serverAccess      = require('../sources/serverAccess')
const factions          = require('../sources/factionWhitelist')

const router = Router()

router.get('/', requirePermission('players.view'), async (_req, res) => {
  res.json({ players: await enrichPlayers(players.list()) })
})

router.post('/', requirePermission('players.manage'), async (req, res) => {
  try {
    const player = players.createManual(req.body || {})
    res.status(201).json(await enrichPlayer(player))
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to create player' })
  }
})

router.get('/:profileId', requirePermission('players.view'), async (req, res) => {
  const player = players.getByProfileId(req.params.profileId)
  if (!player) return res.status(404).json({ error: 'player not found' })
  res.json(await enrichPlayer(player))
})

router.put('/:profileId', requirePermission('players.manage'), async (req, res) => {
  try {
    res.json(await enrichPlayer(players.updateByProfileId(req.params.profileId, req.body || {})))
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to update player' })
  }
})

router.put('/:profileId/whitelist', requirePermission('players.manage'), async (req, res) => {
  await mutateAccess(req, res, 'whitelist')
})

router.put('/:profileId/ban', requirePermission('players.manage'), async (req, res) => {
  await mutateAccess(req, res, 'ban')
})

router.post('/:profileId/factions', requirePermission('factions.manage'), (req, res) => {
  try {
    const discordId = requireDiscordId(req.params.profileId)
    const assignment = factions.createAssignment({
      ...req.body,
      discordId,
    }, req.session.discordId)
    res.status(201).json(assignment)
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to assign faction' })
  }
})

router.delete('/:profileId/factions/:assignmentId', requirePermission('factions.manage'), (req, res) => {
  try {
    const discordId = requireDiscordId(req.params.profileId)
    const belongsToPlayer = factions
      .getPlayerAssignments(discordId)
      .some(assignment => assignment.id === req.params.assignmentId)
    if (!belongsToPlayer) {
      const err = new Error('assignment not found for player')
      err.status = 404
      throw err
    }
    factions.deleteAssignment(req.params.assignmentId)
    res.json({ ok: true })
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to remove faction' })
  }
})

async function mutateAccess(req, res, type) {
  try {
    const discordId = requireDiscordId(req.params.profileId)
    const enabled = req.body && req.body.enabled === true
    const result = type === 'whitelist'
      ? await serverAccess.setWhitelisted(discordId, enabled)
      : await serverAccess.setBanned(discordId, enabled)
    res.json({ ok: true, ...result, player: await enrichPlayer(players.getByProfileId(req.params.profileId)) })
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to update access' })
  }
}

function requireDiscordId(profileId) {
  const discordId = profiles.getDiscordIdByProfileId(profileId)
  if (!discordId) {
    const err = new Error('player not found')
    err.status = 404
    throw err
  }
  return discordId
}

async function enrichPlayers(list) {
  return Promise.all(list.map(enrichPlayer))
}

async function enrichPlayer(player) {
  if (!player) return null
  let access = null
  try {
    const result = await serverAccess.getDiscordAccess(player.discordId)
    access = {
      allowed: result.allowed,
      error: result.error || null,
      roles: result.roles,
    }
  } catch (err) {
    access = { allowed: false, error: 'accessUnavailable', roles: [] }
  }
  return { ...player, access }
}

module.exports = router
