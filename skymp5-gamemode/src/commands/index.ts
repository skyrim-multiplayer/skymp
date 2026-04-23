// ── Commands ──────────────────────────────────────────────────────────────────

import type { Mp, Store, Bus, PlayerState, Sentence } from '../types'
import * as bountyMod from '../systems/social/bounty'
import * as captivity from '../systems/combat/captivity'
import * as college from '../systems/education/college'
import * as combat from '../systems/combat/combat'
import * as drunkBar from '../systems/survival/drunkBar'
import * as economy from '../systems/economy/economy'
import * as factions from '../systems/social/factions'
import * as housing from '../systems/social/housing'
import * as hunger from '../systems/survival/hunger'
import * as nvfl from '../systems/combat/nvfl'
import * as prison from '../systems/justice/prison'
import * as skills from '../systems/education/skills'
import * as training from '../systems/education/training'
import * as chat from '../systems/communication/chat'

// ── Helpers ───────────────────────────────────────────────────────────────────

export function parseCommand(text: string): { cmd: string; args: string[] } | null {
  if (!text || !text.startsWith('/')) return null
  const parts = text.trim().slice(1).split(/\s+/)
  return { cmd: parts[0].toLowerCase(), args: parts.slice(1) }
}

export function findPlayer(store: Store, name: string): PlayerState | null {
  if (!name) return null
  const lower = name.toLowerCase()
  return store.getAll().find(p => p.name.toLowerCase() === lower) ?? null
}

export function checkPermission(store: Store, playerId: number, level: 'player' | 'staff' | 'leader'): boolean {
  if (level === 'player') return true
  const player = store.get(playerId)
  if (!player) return false
  if (level === 'staff')  return player.isStaff
  if (level === 'leader') return player.isLeader || player.isStaff
  return false
}

type CommandHandler = (userId: number, args: string[]) => void

// reply is assigned inside registerAll once we have the chat module.
let reply: (mp_: Mp, store_: Store, playerId: number, message: string) => void = () => {}

// ── Command registration ──────────────────────────────────────────────────────

export function registerAll(mp: Mp, store: Store, bus: Bus): { handle: (userId: number, text: string) => boolean } {
  // Wire reply to the chat module so command responses appear in the UI.
  reply = (mp_, store_, playerId, message) => chat.sendToPlayer(mp_, store_, playerId, message)

  const handlers: Record<string, CommandHandler> = {}

  // ── College ──────────────────────────────────────────────────────────────
  handlers['lecture'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const sub = args[0]
    if (sub === 'start') {
      const ok = college.startLecture(mp, store, bus, userId)
      reply(mp, store, userId, ok ? 'Lecture started.' : 'You already have an active lecture.')
    } else if (sub === 'join') {
      const lecturerId = _findUserIdByName(store, args[1])
      if (lecturerId === null) return reply(mp, store, userId, `Player "${args[1]}" not found.`)
      const ok = college.joinLecture(mp, store, bus, userId, lecturerId)
      reply(mp, store, userId, ok ? 'Joined lecture.' : 'Could not join that lecture.')
    } else if (sub === 'end') {
      const ok = college.endLecture(mp, store, bus, userId)
      reply(mp, store, userId, ok ? 'Lecture ended. XP distributed.' : 'No active lecture.')
    } else {
      reply(mp, store, userId, 'Usage: /lecture start | join [name] | end')
    }
  }

  handlers['study'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const baseId = parseInt(args[0], 16)
    if (!baseId) return reply(mp, store, userId, 'Usage: /study [tomeBaseId]')
    college.studyTome(mp, store, bus, userId, baseId)
    reply(mp, store, userId, 'Studied tome.')
  }

  // ── Training ─────────────────────────────────────────────────────────────
  handlers['train'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const sub = args[0]
    const skillIds = skills.SKILL_IDS as readonly string[]
    if (sub === 'start') {
      const skillId = (args[1] ?? '').toLowerCase()
      if (!skillIds.includes(skillId)) return reply(mp, store, userId, `Valid skills: ${skillIds.join(', ')}`)
      const ok = training.startTraining(mp, store, bus, userId, skillId)
      reply(mp, store, userId, ok ? `Training session started for ${skillId}.` : 'You already have an active session.')
    } else if (sub === 'join') {
      const trainerId = _findUserIdByName(store, args[1])
      if (trainerId === null) return reply(mp, store, userId, `Player "${args[1]}" not found.`)
      const ok = training.joinTraining(mp, store, bus, userId, trainerId)
      reply(mp, store, userId, ok ? 'Joined training session.' : 'Could not join (not nearby or no session).')
    } else if (sub === 'end') {
      const ok = training.endTraining(mp, store, bus, userId)
      reply(mp, store, userId, ok ? 'Training ended. Boosts granted to attendees.' : 'No active session.')
    } else {
      reply(mp, store, userId, 'Usage: /train start [skillId] | join [name] | end')
    }
  }

  // ── Skills ───────────────────────────────────────────────────────────────
  handlers['skill'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const player = store.get(userId)
    if (!player) return
    const target = (args[0] ?? '').toLowerCase()
    const list   = target ? [target] : (skills.SKILL_IDS as readonly string[])
    const lines: string[] = []
    for (const skillId of list) {
      const xp    = skills.getSkillXp(mp, userId, skillId)
      const level = skills.getSkillLevel(xp)
      const cap   = skills.getSkillCap(mp, store, userId, skillId)
      lines.push(`${skillId}: level ${level} (${xp}/${cap} XP)`)
    }
    reply(mp, store, userId, lines.join('\n'))
  }

  // ── Economy ──────────────────────────────────────────────────────────────
  handlers['pay'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const amount = parseInt(args[0])
    if (!amount || amount <= 0) return reply(mp, store, userId, 'Usage: /pay [amount] [playerName]')
    const target = findPlayer(store, args[1])
    if (!target) return reply(mp, store, userId, `Player "${args[1]}" not found.`)
    const ok = economy.transferGold(mp, store, userId, target.id, amount)
    if (ok) {
      reply(mp, store, userId, `Paid ${amount} Septims to ${target.name}.`)
      reply(mp, store, target.id, `Received ${amount} Septims from ${store.get(userId)!.name}.`)
    } else {
      reply(mp, store, userId, 'Insufficient funds.')
    }
  }

  // ── Housing ──────────────────────────────────────────────────────────────
  handlers['property'] = (userId, args) => {
    const sub = args[0]
    if (sub === 'list') {
      if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
      const player = store.get(userId)
      const holdId = player ? player.holdId : null
      if (!holdId) return reply(mp, store, userId, 'You are not assigned to a hold.')
      const list  = housing.getPropertiesByHold(holdId)
      const lines = list.map(p => `${p.id}: ${p.name} [${p.type}] — ${p.ownerId ? 'Owned' : p.pendingOwnerId ? 'Pending' : 'Available'}`)
      reply(mp, store, userId, lines.length ? lines.join('\n') : 'No properties in this hold.')
    } else if (sub === 'request') {
      if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
      const propertyId = args[1]
      if (!propertyId) return reply(mp, store, userId, 'Usage: /property request [propertyId]')
      const stewardId = _findStewardForProperty(store, propertyId)
      if (stewardId === null) return reply(mp, store, userId, 'No Steward available in this hold.')
      const ok = housing.requestProperty(mp, store, bus, userId, propertyId, stewardId)
      reply(mp, store, userId, ok ? 'Property request sent to Steward.' : 'Property unavailable.')
    } else if (sub === 'approve') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const propertyId = args[1]
      const ok = housing.approveProperty(mp, store, bus, propertyId, userId)
      reply(mp, store, userId, ok ? 'Property approved.' : 'No pending request for that property.')
    } else if (sub === 'deny') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const propertyId = args[1]
      const ok = housing.denyProperty(mp, propertyId)
      reply(mp, store, userId, ok ? 'Property request denied.' : 'Property not found.')
    } else if (sub === 'revoke') {
      if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
      const propertyId = args[1]
      const ok = housing.revokeProperty(mp, store, propertyId)
      reply(mp, store, userId, ok ? 'Property revoked.' : 'Property not found.')
    } else {
      reply(mp, store, userId, 'Usage: /property list | request [id] | approve [id] | deny [id] | revoke [id]')
    }
  }

  // ── Bounty ───────────────────────────────────────────────────────────────
  handlers['bounty'] = (userId, args) => {
    const sub = args[0]
    if (!sub) {
      if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
      const bounties = bountyMod.getAllBounties(mp, store, userId)
      const lines = Object.entries(bounties).filter(([, v]) => v > 0).map(([h, v]) => `${h}: ${v}`)
      reply(mp, store, userId, lines.length ? lines.join('\n') : 'No bounties.')
    } else if (sub === 'check') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target = findPlayer(store, args[1])
      if (!target) return reply(mp, store, userId, `Player "${args[1]}" not found.`)
      const bounties = bountyMod.getAllBounties(mp, store, target.id)
      const lines = Object.entries(bounties).filter(([, v]) => v > 0).map(([h, v]) => `${h}: ${v}`)
      reply(mp, store, userId, `Bounties for ${target.name}:\n${lines.length ? lines.join('\n') : 'None'}`)
    } else if (sub === 'add') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target = findPlayer(store, args[1])
      const holdId = (args[2] ?? '').toLowerCase()
      const amount = parseInt(args[3])
      if (!target || !holdId || !amount) return reply(mp, store, userId, 'Usage: /bounty add [name] [holdId] [amount]')
      bountyMod.addBounty(mp, store, bus, target.id, holdId, amount)
      reply(mp, store, userId, `Added ${amount} bounty for ${target.name} in ${holdId}.`)
    } else if (sub === 'clear') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target = findPlayer(store, args[1])
      const holdId = (args[2] ?? '').toLowerCase()
      if (!target || !holdId) return reply(mp, store, userId, 'Usage: /bounty clear [name] [holdId]')
      bountyMod.clearBounty(mp, store, bus, target.id, holdId)
      reply(mp, store, userId, `Cleared bounty for ${target.name} in ${holdId}.`)
    } else {
      reply(mp, store, userId, 'Usage: /bounty | check [name] | add [name] [hold] [amount] | clear [name] [hold]')
    }
  }

  // ── Justice ──────────────────────────────────────────────────────────────
  handlers['arrest'] = (userId, args) => {
    if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
    const target  = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    const officer = store.get(userId)
    const holdId  = officer ? officer.holdId : null
    if (!holdId) return reply(mp, store, userId, 'You are not assigned to a hold.')
    const jarlId = _findJarlForHold(store, holdId)
    const ok = prison.queueForSentencing(mp, store, bus, target.id, holdId, userId, jarlId ?? userId)
    reply(mp, store, userId, ok ? `${target.name} queued for sentencing.` : `${target.name} is already in queue.`)
  }

  handlers['sentence'] = (userId, args) => {
    if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    const type = (args[1] ?? '').toLowerCase()
    if (!['fine','release','banish'].includes(type)) return reply(mp, store, userId, 'Usage: /sentence [name] fine [amount] | release | banish')
    const sentence: Sentence = { type: type as Sentence['type'] }
    if (type === 'fine') sentence.fineAmount = parseInt(args[2]) || 0
    const ok = prison.sentencePlayer(mp, store, bus, target.id, userId, sentence)
    reply(mp, store, userId, ok ? `Sentenced ${target.name}: ${type}.` : `${target.name} is not in queue.`)
  }

  // ── Captivity ────────────────────────────────────────────────────────────
  handlers['capture'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    if (!target.isDown) return reply(mp, store, userId, `${target.name} is not downed.`)
    captivity.capturePlayer(mp, store, bus, target.id, userId)
    reply(mp, store, userId, `${target.name} taken captive.`)
  }

  handlers['release'] = (userId, args) => {
    if (!checkPermission(store, userId, 'player')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    captivity.releasePlayer(mp, store, bus, target.id)
    reply(mp, store, userId, `${target.name} released.`)
  }

  // ── Combat (staff) ───────────────────────────────────────────────────────
  handlers['down'] = (userId, args) => {
    if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    combat.downPlayer(mp, store, bus, target.id, userId)
    reply(mp, store, userId, `${target.name} forced down.`)
  }

  handlers['rise'] = (userId, args) => {
    if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    combat.risePlayer(mp, store, bus, target.id)
    reply(mp, store, userId, `${target.name} risen.`)
  }

  handlers['nvfl'] = (userId, args) => {
    if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
    if (args[0] === 'clear') {
      const target = findPlayer(store, args[1])
      if (!target) return reply(mp, store, userId, `Player "${args[1]}" not found.`)
      nvfl.clearNvfl(store, target.id)
      reply(mp, store, userId, `NVFL cleared for ${target.name}.`)
    } else {
      reply(mp, store, userId, 'Usage: /nvfl clear [name]')
    }
  }

  // ── Factions ─────────────────────────────────────────────────────────────
  handlers['faction'] = (userId, args) => {
    const sub = args[0]
    if (sub === 'join') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target    = findPlayer(store, args[1])
      const factionId = (args[2] ?? '').toLowerCase()
      const rank      = args[3] !== undefined ? parseInt(args[3]) : 0
      if (!target || !factionId) return reply(mp, store, userId, 'Usage: /faction join [name] [factionId] (rank)')
      factions.joinFaction(mp, store, bus, target.id, factionId, rank)
      reply(mp, store, userId, `${target.name} joined ${factionId} at rank ${rank}.`)
    } else if (sub === 'leave') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target    = findPlayer(store, args[1])
      const factionId = (args[2] ?? '').toLowerCase()
      if (!target || !factionId) return reply(mp, store, userId, 'Usage: /faction leave [name] [factionId]')
      factions.leaveFaction(mp, store, bus, target.id, factionId)
      reply(mp, store, userId, `${target.name} left ${factionId}.`)
    } else if (sub === 'rank') {
      if (!checkPermission(store, userId, 'leader')) return reply(mp, store, userId, 'No permission.')
      const target    = findPlayer(store, args[1])
      const factionId = (args[2] ?? '').toLowerCase()
      const rank      = parseInt(args[3])
      if (!target || !factionId || isNaN(rank)) return reply(mp, store, userId, 'Usage: /faction rank [name] [factionId] [rank]')
      factions.joinFaction(mp, store, bus, target.id, factionId, rank)
      reply(mp, store, userId, `${target.name} set to rank ${rank} in ${factionId}.`)
    } else if (sub === 'bbb') {
      if (args[1] === 'set') {
        if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
        reply(mp, store, userId, 'BBB set not yet implemented (requires multi-line input).')
      } else {
        const factionId = (args[1] ?? '').toLowerCase()
        const doc = factions.getFactionDocument(mp, factionId)
        if (!doc) return reply(mp, store, userId, `No BBB document for ${factionId}.`)
        reply(mp, store, userId, `[${factionId}] Benefits: ${doc.benefits}\nBurdens: ${doc.burdens}\nBylaws: ${doc.bylaws}`)
      }
    } else {
      reply(mp, store, userId, 'Usage: /faction join|leave|rank|bbb ...')
    }
  }

  // ── Staff utilities ──────────────────────────────────────────────────────
  handlers['sober'] = (userId, args) => {
    if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    drunkBar.soberPlayer(mp, store, bus, target.id)
    reply(mp, store, userId, `${target.name} sobered.`)
  }

  handlers['feed'] = (userId, args) => {
    if (!checkPermission(store, userId, 'staff')) return reply(mp, store, userId, 'No permission.')
    const target = findPlayer(store, args[0])
    if (!target) return reply(mp, store, userId, `Player "${args[0]}" not found.`)
    const levels = parseInt(args[1]) || 5
    hunger.feedPlayer(mp, store, bus, target.id, levels)
    reply(mp, store, userId, `Fed ${target.name} (${levels} levels).`)
  }

  console.log(`[commands] Registered ${Object.keys(handlers).length} commands`)

  function handle(userId: number, text: string): boolean {
    const parsed = parseCommand(text)
    if (!parsed) return false
    const handler = handlers[parsed.cmd]
    if (!handler) {
      reply(mp, store, userId, `Unknown command: /${parsed.cmd}`)
      return true
    }
    try {
      handler(userId, parsed.args)
    } catch (err: any) {
      console.error(`[commands] Error in /${parsed.cmd} for ${userId}: ${err.message}`)
      reply(mp, store, userId, 'Command error — see server log.')
    }
    return true
  }

  return { handle }
}

// ── Private helpers ───────────────────────────────────────────────────────────

function _findUserIdByName(store: Store, name: string): number | null {
  const player = store.getAll().find(p => p.name.toLowerCase() === (name ?? '').toLowerCase())
  return player ? player.id : null
}

function _findStewardForProperty(store: Store, propertyId: string): number | null {
  const prop = housing.getProperty(propertyId)
  if (!prop) return null
  const candidates = store.getAll().filter(p => p.holdId === prop.holdId && p.isLeader)
  return candidates.length ? candidates[0].id : null
}

function _findJarlForHold(store: Store, holdId: string): number | null {
  const candidates = store.getAll().filter(p => p.holdId === holdId && p.isLeader)
  return candidates.length ? candidates[0].id : null
}
