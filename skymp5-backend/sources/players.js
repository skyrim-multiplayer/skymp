'use strict'

const fs               = require('fs')
const path             = require('path')
const profiles         = require('./profiles')
const factionWhitelist = require('./factionWhitelist')

const FILE = path.join(__dirname, '..', 'data', 'players.json')

function load() {
  try {
    const data = JSON.parse(fs.readFileSync(FILE, 'utf8'))
    return data && typeof data === 'object' && !Array.isArray(data) ? data : {}
  } catch {
    return {}
  }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2) + '\n')
}

function upsertFromDiscordUser(discordUser) {
  if (!discordUser || !discordUser.id) throw new Error('discordUser.id is required')
  const discordId = String(discordUser.id)
  const profileId = profiles.getOrCreateProfileId(discordId)
  const data = load()
  const existing = data[discordId] || {}
  const now = new Date().toISOString()

  data[discordId] = {
    profileId,
    discordId,
    username: discordUser.username || existing.username || '',
    displayName: discordUser.global_name || discordUser.displayName || discordUser.username || existing.displayName || '',
    avatar: discordUser.avatar || existing.avatar || null,
    notes: existing.notes || '',
    createdAt: existing.createdAt || now,
    updatedAt: now,
    lastSeenAt: now,
  }

  save(data)
  return data[discordId]
}

function createManual(input) {
  const discordId = String(input.discordId || '').trim()
  if (!discordId) {
    const err = new Error('discordId is required')
    err.status = 400
    throw err
  }
  const profileId = profiles.getOrCreateProfileId(discordId)
  const data = load()
  const existing = data[discordId] || {}
  const now = new Date().toISOString()

  data[discordId] = {
    profileId,
    discordId,
    username: String(input.username || existing.username || '').trim(),
    displayName: String(input.displayName || existing.displayName || input.username || '').trim(),
    avatar: existing.avatar || null,
    notes: String(input.notes || existing.notes || '').trim(),
    createdAt: existing.createdAt || now,
    updatedAt: now,
    lastSeenAt: existing.lastSeenAt || null,
  }

  save(data)
  return decorate(data[discordId])
}

function updateByProfileId(profileId, patch) {
  const discordId = profiles.getDiscordIdByProfileId(profileId)
  if (!discordId) {
    const err = new Error('player not found')
    err.status = 404
    throw err
  }

  const data = load()
  const current = data[discordId] || { profileId: Number(profileId), discordId }
  if (patch.username !== undefined) current.username = String(patch.username || '').trim()
  if (patch.displayName !== undefined) current.displayName = String(patch.displayName || '').trim()
  if (patch.notes !== undefined) current.notes = String(patch.notes || '').trim()
  current.updatedAt = new Date().toISOString()
  data[discordId] = current
  save(data)
  return decorate(current)
}

function list() {
  const data = load()
  return profiles.list().map(profile => {
    const row = data[profile.discordId] || {
      profileId: profile.profileId,
      discordId: profile.discordId,
      username: '',
      displayName: '',
      avatar: null,
      notes: '',
      createdAt: null,
      updatedAt: null,
      lastSeenAt: null,
    }
    return decorate(row)
  })
}

function getByProfileId(profileId) {
  const discordId = profiles.getDiscordIdByProfileId(profileId)
  if (!discordId) return null
  const data = load()
  return decorate(data[discordId] || { profileId: Number(profileId), discordId })
}

function decorate(player) {
  return {
    ...player,
    profileId: Number(player.profileId),
    assignments: factionWhitelist.getPlayerAssignments(player.discordId),
    factionPermissions: factionWhitelist.getPlayerFactionPermissions(player.discordId),
    gameFactions: factionWhitelist.getPlayerGameFactions(player.discordId),
  }
}

module.exports = {
  load,
  save,
  list,
  getByProfileId,
  upsertFromDiscordUser,
  createManual,
  updateByProfileId,
}
