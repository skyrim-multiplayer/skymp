'use strict'

const { Router }        = require('express')
const requirePermission = require('../middleware/requirePermission')
const permissions       = require('../sources/permissions')

const router = Router()

const KNOWN_PERMISSIONS = [
  'admin.*',
  'dashboard.access',
  'permissions.manage',
  'players.view',
  'players.manage',
  'server.access.view',
  'server.access.manage',
  'factions.view',
  'factions.manage',
  'lore.write',
  'rules.write',
  'staff.whitelist_info',
]

router.get('/', requirePermission('permissions.manage'), (_req, res) => {
  res.json({
    roles: permissions.listRolePermissions().roles || {},
    knownPermissions: KNOWN_PERMISSIONS,
  })
})

router.put('/:roleId', requirePermission('permissions.manage'), (req, res) => {
  try {
    const { name, permissions: rolePermissions } = req.body || {}
    const role = permissions.setRolePermissions(req.params.roleId, name, rolePermissions)
    res.json(role)
  } catch (err) {
    res.status(err.status || 500).json({ error: err.message || 'failed to save role permissions' })
  }
})

router.delete('/:roleId', requirePermission('permissions.manage'), (req, res) => {
  permissions.deleteRolePermissions(req.params.roleId)
  res.json({ ok: true })
})

module.exports = router
