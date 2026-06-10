'use strict'

const fs = require('fs')
const path = require('path')
const config = require('../../config')
const discordBot = require('../discord/bot')

const FILE = path.join(__dirname, '..', '..', 'data', 'server-access.json')
const WHITELIST_PATH = path.join(__dirname, '..', '..', 'data', 'whitelist.json')

function uniq(values) {
  return [...new Set((values || []).map(v => String(v || '').trim()).filter(Boolean))]
}

function defaults() {
  return {
    serverLocked: config.serverLocked,
    lockedRoleIds: uniq(config.serverLockedRoleIds),
    lockedDiscordIds: uniq(config.serverLockedAllowList),
    whitelistRoleId: config.whitelistRoleId || '',
    bannedRoleId: config.bannedRoleId || '',
  }
}

function load() {
  const base = defaults()
  try {
    const file = JSON.parse(fs.readFileSync(FILE, 'utf8'))
    return normalize(applyEnvFallbacks({ ...base, ...file }, base))
  } catch {
    return normalize(base)
  }
}

function applyEnvFallbacks(settings, base) {
  return {
    ...settings,
    whitelistRoleId: settings.whitelistRoleId || base.whitelistRoleId,
    bannedRoleId: settings.bannedRoleId || base.bannedRoleId,
  }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(normalize(data), null, 2) + '\n')
}

function normalize(data) {
  return {
    serverLocked: data.serverLocked === true,
    lockedRoleIds: uniq(data.lockedRoleIds),
    lockedDiscordIds: uniq(data.lockedDiscordIds),
    whitelistRoleId: String(data.whitelistRoleId || '').trim(),
    bannedRoleId: String(data.bannedRoleId || '').trim(),
  }
}

function update(input) {
  const current = load()
  const next = normalize({
    ...current,
    ...(input || {}),
    lockedRoleIds: input && input.lockedRoleIds !== undefined ? input.lockedRoleIds : current.lockedRoleIds,
    lockedDiscordIds: input && input.lockedDiscordIds !== undefined ? input.lockedDiscordIds : current.lockedDiscordIds,
  })
  save(next)
  return next
}

function loadFileWhitelist() {
  try { return JSON.parse(fs.readFileSync(WHITELIST_PATH, 'utf8')) }
  catch { return [] }
}

function saveFileWhitelist(discordIds) {
  fs.writeFileSync(WHITELIST_PATH, JSON.stringify(uniq(discordIds), null, 2) + '\n')
}

function hasAnyRole(memberRoleIds, requiredRoleIds) {
  const roles = new Set(memberRoleIds || [])
  return (requiredRoleIds || []).some(roleId => roles.has(roleId))
}

async function getDiscordAccess(discordId) {
  const settings = load()
  const roles = await discordBot.getMemberRoles(discordId)

  if (settings.bannedRoleId && roles.includes(settings.bannedRoleId)) {
    return { allowed: false, error: 'banned', roles, settings }
  }

  if (settings.serverLocked) {
    if (settings.lockedDiscordIds.includes(discordId) || hasAnyRole(roles, settings.lockedRoleIds)) {
      return { allowed: true, roles, settings }
    }
    return { allowed: false, error: 'serverLocked', roles, settings }
  }

  if (settings.whitelistRoleId) {
    if (!roles.includes(settings.whitelistRoleId)) {
      return { allowed: false, error: 'notWhitelisted', roles, settings }
    }
    return { allowed: true, roles, settings }
  }

  const fileWhitelist = loadFileWhitelist()
  if (fileWhitelist.length > 0 && !fileWhitelist.includes(discordId)) {
    return { allowed: false, error: 'notWhitelisted', roles, settings }
  }

  return { allowed: true, roles, settings }
}

function publicState() {
  const settings = load()
  return {
    ...settings,
    legacyFileWhitelistCount: loadFileWhitelist().length,
  }
}

async function setWhitelisted(discordId, enabled) {
  const settings = load()
  if (settings.whitelistRoleId) {
    if (enabled) await discordBot.addMemberRole(discordId, settings.whitelistRoleId)
    else await discordBot.removeMemberRole(discordId, settings.whitelistRoleId)
    return { source: 'discord-role', roleId: settings.whitelistRoleId, whitelisted: enabled }
  }

  const current = new Set(loadFileWhitelist())
  if (enabled) current.add(discordId)
  else current.delete(discordId)
  saveFileWhitelist([...current])
  return { source: 'file', roleId: null, whitelisted: enabled }
}

async function setBanned(discordId, enabled) {
  const settings = load()
  if (!settings.bannedRoleId) {
    const err = new Error('bannedRoleId is not configured')
    err.status = 400
    throw err
  }
  if (enabled) await discordBot.addMemberRole(discordId, settings.bannedRoleId)
  else await discordBot.removeMemberRole(discordId, settings.bannedRoleId)
  return { source: 'discord-role', roleId: settings.bannedRoleId, banned: enabled }
}

module.exports = {
  load,
  update,
  publicState,
  getDiscordAccess,
  hasAnyRole,
  setWhitelisted,
  setBanned,
}
