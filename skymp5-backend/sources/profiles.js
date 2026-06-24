'use strict'

const fs   = require('fs')
const path = require('path')

const FILE = path.join(__dirname, '..', 'data', 'profiles.json')

function load() {
  try {
    const data = JSON.parse(fs.readFileSync(FILE, 'utf8'))
    return {
      nextId: Number.isInteger(data.nextId) ? data.nextId : 1,
      map: data.map && typeof data.map === 'object' ? data.map : {},
    }
  } catch {
    return { nextId: 1, map: {} }
  }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2) + '\n')
}

function getOrCreateProfileId(discordId) {
  const id = String(discordId || '').trim()
  if (!id) throw new Error('discordId is required')

  const data = load()
  if (!data.map[id]) {
    data.map[id] = data.nextId++
    save(data)
  }
  return data.map[id]
}

function getDiscordIdByProfileId(profileId) {
  const id = Number(profileId)
  const entry = Object.entries(load().map).find(([, value]) => value === id)
  return entry ? entry[0] : null
}

function list() {
  return Object.entries(load().map)
    .map(([discordId, profileId]) => ({ discordId, profileId }))
    .sort((a, b) => a.profileId - b.profileId)
}

module.exports = {
  load,
  save,
  list,
  getOrCreateProfileId,
  getDiscordIdByProfileId,
}
