'use strict'

const crypto = require('crypto')
const fs     = require('fs')
const path   = require('path')

const FILE = path.join(__dirname, '..', 'data', 'faction-whitelist.json')

function load() {
  try {
    const data = JSON.parse(fs.readFileSync(FILE, 'utf8'))
    return {
      requirements: Array.isArray(data.requirements) ? data.requirements : [],
      assignments: Array.isArray(data.assignments) ? data.assignments : [],
    }
  } catch {
    return { requirements: [], assignments: [] }
  }
}

function save(data) {
  fs.writeFileSync(FILE, JSON.stringify(data, null, 2) + '\n')
}

function getRequirement(data, requirementId) {
  return data.requirements.find(req => req.id === requirementId)
}

function normalizeDiscordId(discordId) {
  return String(discordId || '').trim()
}

function list() {
  const data = load()
  const assignmentCounts = data.assignments.reduce((counts, assignment) => {
    counts[assignment.requirementId] = (counts[assignment.requirementId] || 0) + 1
    return counts
  }, {})

  return {
    requirements: data.requirements.map(req => ({
      ...req,
      assigned: assignmentCounts[req.id] || 0,
      remaining: req.capacity === null ? null : Math.max(0, req.capacity - (assignmentCounts[req.id] || 0)),
    })),
    assignments: data.assignments,
  }
}

function createAssignment(input, actorId) {
  const data = load()
  const requirement = getRequirement(data, input.requirementId)
  if (!requirement) {
    const err = new Error('unknown requirement')
    err.status = 400
    throw err
  }

  const discordId = normalizeDiscordId(input.discordId)
  if (!discordId) {
    const err = new Error('discordId is required')
    err.status = 400
    throw err
  }

  if (data.assignments.some(item => item.requirementId === requirement.id && item.discordId === discordId)) {
    const err = new Error('player already has this rank')
    err.status = 409
    throw err
  }

  if (requirement.capacity !== null) {
    const count = data.assignments.filter(item => item.requirementId === requirement.id).length
    if (count >= requirement.capacity) {
      const err = new Error('slot is already filled')
      err.status = 409
      throw err
    }
  }

  const now = new Date().toISOString()
  const assignment = {
    id: crypto.randomUUID(),
    requirementId: requirement.id,
    discordId,
    playerName: String(input.playerName || '').trim(),
    notes: String(input.notes || '').trim(),
    createdAt: now,
    createdBy: actorId || null,
    updatedAt: now,
    updatedBy: actorId || null,
  }

  data.assignments.push(assignment)
  save(data)
  return decorateAssignment(assignment, requirement)
}

function updateAssignment(id, input, actorId) {
  const data = load()
  const idx = data.assignments.findIndex(item => item.id === id)
  if (idx === -1) {
    const err = new Error('assignment not found')
    err.status = 404
    throw err
  }

  const assignment = data.assignments[idx]
  if (input.playerName !== undefined) assignment.playerName = String(input.playerName || '').trim()
  if (input.notes !== undefined) assignment.notes = String(input.notes || '').trim()
  if (input.discordId !== undefined) {
    const discordId = normalizeDiscordId(input.discordId)
    if (!discordId) {
      const err = new Error('discordId is required')
      err.status = 400
      throw err
    }
    const duplicate = data.assignments.some(item =>
      item.id !== id && item.requirementId === assignment.requirementId && item.discordId === discordId
    )
    if (duplicate) {
      const err = new Error('player already has this rank')
      err.status = 409
      throw err
    }
    assignment.discordId = discordId
  }

  assignment.updatedAt = new Date().toISOString()
  assignment.updatedBy = actorId || null
  save(data)
  return decorateAssignment(assignment, getRequirement(data, assignment.requirementId))
}

function deleteAssignment(id) {
  const data = load()
  const idx = data.assignments.findIndex(item => item.id === id)
  if (idx === -1) {
    const err = new Error('assignment not found')
    err.status = 404
    throw err
  }
  data.assignments.splice(idx, 1)
  save(data)
}

function decorateAssignment(assignment, requirement) {
  return {
    ...assignment,
    requirement: requirement || null,
    permission: requirement ? requirement.permission : null,
  }
}

function getPlayerFactionPermissions(discordId) {
  const data = load()
  const normalized = normalizeDiscordId(discordId)
  const byId = new Map(data.requirements.map(req => [req.id, req]))
  return data.assignments
    .filter(assignment => assignment.discordId === normalized)
    .map(assignment => byId.get(assignment.requirementId))
    .filter(Boolean)
    .map(req => req.permission)
}

function getPlayerGameFactions(discordId) {
  return getPlayerAssignments(discordId)
    .filter(assignment => assignment.requirement)
    .map(assignment => {
      const req = assignment.requirement
      return {
        factionId: `${req.scope}:${slug(req.group)}`,
        rank: req.capacity === 1 ? 100 : 0,
        title: req.rank,
        permission: req.permission,
        scope: req.scope,
        group: req.group,
      }
    })
}

function getPlayerAssignments(discordId) {
  const data = load()
  const normalized = normalizeDiscordId(discordId)
  const byId = new Map(data.requirements.map(req => [req.id, req]))
  return data.assignments
    .filter(assignment => assignment.discordId === normalized)
    .map(assignment => decorateAssignment(assignment, byId.get(assignment.requirementId)))
}

function slug(value) {
  return String(value || '')
    .toLowerCase()
    .replace(/[^a-z0-9]+/g, '-')
    .replace(/^-+|-+$/g, '')
}

module.exports = {
  list,
  createAssignment,
  updateAssignment,
  deleteAssignment,
  getPlayerFactionPermissions,
  getPlayerGameFactions,
  getPlayerAssignments,
}
