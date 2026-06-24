'use strict'
// ── Permission resolution ─────────────────────────────────────────────────────
// Maps Discord role IDs to flat permission strings using data/role-permissions.json.

const fs   = require('fs')
const path = require('path')

const FILE = path.join(__dirname, '..', 'data', 'role-permissions.json')

function _load() {
  try {
    return JSON.parse(fs.readFileSync(FILE, 'utf8'))
  } catch {
    return { roles: {} }
  }
}

function _save(config) {
  fs.writeFileSync(FILE, JSON.stringify(config, null, 2) + '\n')
}

function listRolePermissions() {
  return _load()
}

function setRolePermissions(roleId, name, permissions) {
  const normalizedRoleId = String(roleId || '').trim()
  if (!normalizedRoleId) {
    const err = new Error('roleId is required')
    err.status = 400
    throw err
  }
  if (!Array.isArray(permissions)) {
    const err = new Error('permissions must be an array')
    err.status = 400
    throw err
  }

  const config = _load()
  if (!config.roles) config.roles = {}
  config.roles[normalizedRoleId] = {
    name: String(name || normalizedRoleId).trim(),
    permissions: [...new Set(permissions.map(p => String(p || '').trim()).filter(Boolean))],
  }
  _save(config)
  return config.roles[normalizedRoleId]
}

function deleteRolePermissions(roleId) {
  const config = _load()
  if (config.roles) delete config.roles[String(roleId || '').trim()]
  _save(config)
}

/**
 * Given an array of Discord role ID strings, returns a deduplicated flat array
 * of permission strings based on the role-permissions config.
 * @param {string[]} roleIds
 * @returns {string[]}
 */
function resolvePermissions(roleIds) {
  const config = _load()
  const perms  = new Set()
  for (const roleId of roleIds) {
    const entry = config.roles[roleId]
    if (entry) entry.permissions.forEach(p => perms.add(p))
  }
  return [...perms]
}

/**
 * Returns true if the given permissions array grants the required permission.
 * 'admin.*' is a wildcard that grants every permission.
 * @param {string[]} permissions
 * @param {string} required
 * @returns {boolean}
 */
function hasPermission(permissions, required) {
  if (permissions.includes('admin.*')) return true
  return permissions.includes(required)
}

module.exports = {
  listRolePermissions,
  setRolePermissions,
  deleteRolePermissions,
  resolvePermissions,
  hasPermission,
}
