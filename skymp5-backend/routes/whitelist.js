'use strict'
// Role-backed whitelist API. Read-only for now: Discord roles are the source
// of truth, and future character/race locks can hang off this response shape.

const { Router }        = require('express')
const discordBot        = require('../sources/discordBot')
const requirePermission = require('../middleware/requirePermission')
const serverAccess      = require('../sources/serverAccess')

const router = Router()

router.get('/', requirePermission('staff.whitelist_info'), async (_req, res) => {
  const access = serverAccess.load()
  if (!access.whitelistRoleId) {
    return res.json({
      source: 'file',
      roleId: null,
      players: [],
      message: 'WHITELIST_ROLE_ID is not configured.',
    })
  }

  try {
    const members = await discordBot.getMembersWithRole(access.whitelistRoleId)
    res.json({
      source: 'discord-role',
      roleId: access.whitelistRoleId,
      count: members.length,
      players: members.map(member => ({
        ...member,
        racePreset: null,
        characterName: null,
      })),
    })
  } catch (err) {
    console.error('[whitelist] failed to list members:', err.message)
    res.status(503).json({ error: 'whitelist unavailable' })
  }
})

module.exports = router
