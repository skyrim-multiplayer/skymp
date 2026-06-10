const config = window.SKYRP_DASHBOARD_CONFIG || {}
const apiBaseUrl = (config.apiBaseUrl || '').replace(/\/$/, '')
const tokenKey = 'skyrp.dashboard.token'

const state = {
  token: localStorage.getItem(tokenKey) || '',
  user: null,
  players: [],
  selectedProfileId: null,
  requirements: [],
  assignments: [],
  access: null,
  roles: {},
  knownPermissions: [],
  selectedRequirementId: '',
  activeView: 'access',
}

const el = id => document.getElementById(id)

const nodes = {
  apiStatus: el('apiStatus'),
  statusText: el('statusText'),
  userName: el('userName'),
  loginButton: el('loginButton'),
  logoutButton: el('logoutButton'),
  signedOut: el('signedOut'),
  accessView: el('accessView'),
  playersView: el('playersView'),
  factionsView: el('factionsView'),
  permissionsView: el('permissionsView'),
  accessForm: el('accessForm'),
  serverLockedInput: el('serverLockedInput'),
  lockedRoleIdsInput: el('lockedRoleIdsInput'),
  lockedDiscordIdsInput: el('lockedDiscordIdsInput'),
  whitelistRoleIdInput: el('whitelistRoleIdInput'),
  bannedRoleIdInput: el('bannedRoleIdInput'),
  accessCheckForm: el('accessCheckForm'),
  accessCheckDiscordId: el('accessCheckDiscordId'),
  accessCheckResult: el('accessCheckResult'),
  playerCount: el('playerCount'),
  playerSearchInput: el('playerSearchInput'),
  newPlayerButton: el('newPlayerButton'),
  playersRefreshButton: el('playersRefreshButton'),
  playersTable: el('playersTable'),
  selectedPlayerLabel: el('selectedPlayerLabel'),
  playerForm: el('playerForm'),
  playerDiscordIdInput: el('playerDiscordIdInput'),
  playerUsernameInput: el('playerUsernameInput'),
  playerDisplayNameInput: el('playerDisplayNameInput'),
  playerNotesInput: el('playerNotesInput'),
  whitelistPlayerButton: el('whitelistPlayerButton'),
  banPlayerButton: el('banPlayerButton'),
  playerAssignmentsTable: el('playerAssignmentsTable'),
  playerFactionForm: el('playerFactionForm'),
  playerRequirementSelect: el('playerRequirementSelect'),
  playerFactionNotesInput: el('playerFactionNotesInput'),
  stats: el('stats'),
  scopeFilter: el('scopeFilter'),
  groupFilter: el('groupFilter'),
  refreshButton: el('refreshButton'),
  requirementsTable: el('requirementsTable'),
  assignmentsTable: el('assignmentsTable'),
  requirementSelect: el('requirementSelect'),
  selectedSlot: el('selectedSlot'),
  slotCount: el('slotCount'),
  assignmentCount: el('assignmentCount'),
  assignmentForm: el('assignmentForm'),
  discordIdInput: el('discordIdInput'),
  playerNameInput: el('playerNameInput'),
  notesInput: el('notesInput'),
  rolesTable: el('rolesTable'),
  roleCount: el('roleCount'),
  roleForm: el('roleForm'),
  roleIdInput: el('roleIdInput'),
  roleNameInput: el('roleNameInput'),
  selectedRole: el('selectedRole'),
  permissionChecks: el('permissionChecks'),
  toast: el('toast'),
}

function toast(message) {
  nodes.toast.textContent = message
  nodes.toast.classList.remove('hidden')
  clearTimeout(toast.timer)
  toast.timer = setTimeout(() => nodes.toast.classList.add('hidden'), 3200)
}

function escapeHtml(value) {
  return String(value ?? '')
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#039;')
}

async function api(path, options = {}) {
  const res = await fetch(`${apiBaseUrl}${path}`, {
    ...options,
    headers: {
      'Content-Type': 'application/json',
      ...(state.token ? { Authorization: `Bearer ${state.token}` } : {}),
      ...(options.headers || {}),
    },
  })

  const text = await res.text()
  const data = text ? JSON.parse(text) : null
  if (!res.ok) {
    throw new Error(data?.error || `Request failed with ${res.status}`)
  }
  return data
}

function captureTokenFromUrl() {
  const url = new URL(window.location.href)
  const token = url.searchParams.get('token')
  if (!token) return
  state.token = token
  localStorage.setItem(tokenKey, token)
  url.searchParams.delete('token')
  url.searchParams.delete('error')
  window.history.replaceState({}, '', url.pathname)
}

async function login() {
  const redirect = `${window.location.origin}/`
  const data = await fetch(`${apiBaseUrl}/auth/dashboard/url?redirect=${encodeURIComponent(redirect)}`)
    .then(res => res.json())
  if (!data.url) throw new Error(data.error || 'OAuth URL unavailable')
  window.location.href = data.url
}

async function logout() {
  try {
    if (state.token) await api('/auth/dashboard/logout', { method: 'POST' })
  } catch {
    // Local logout still matters even if the session already expired.
  }
  state.token = ''
  state.user = null
  localStorage.removeItem(tokenKey)
  renderAuth()
}

async function loadSession() {
  if (!state.token) return
  const data = await api('/auth/dashboard/me')
  state.user = data.user
}

async function loadFactions() {
  const data = await api('/api/faction-whitelist')
  state.requirements = data.requirements || []
  state.assignments = data.assignments || []
}

async function loadPlayers() {
  const data = await api('/api/players')
  state.players = data.players || []
}

async function loadAccess() {
  state.access = await api('/api/server-access')
}

async function loadPermissions() {
  const data = await api('/api/role-permissions')
  state.roles = data.roles || {}
  state.knownPermissions = data.knownPermissions || []
}

function renderAuth() {
  const signedIn = !!state.user
  nodes.apiStatus.textContent = signedIn ? 'Online' : 'Offline'
  nodes.apiStatus.classList.toggle('online', signedIn)
  nodes.userName.textContent = signedIn ? state.user.username : 'Signed out'
  nodes.statusText.textContent = signedIn
    ? `Signed in as ${state.user.username}`
    : 'Connect with Discord to manage the realm.'
  nodes.loginButton.classList.toggle('hidden', signedIn)
  nodes.logoutButton.classList.toggle('hidden', !signedIn)
  nodes.signedOut.classList.toggle('hidden', signedIn)
  document.querySelectorAll('.view').forEach(view => view.classList.add('hidden'))
  if (signedIn) renderActiveView()
}

function renderActiveView() {
  nodes.accessView.classList.toggle('hidden', state.activeView !== 'access')
  nodes.playersView.classList.toggle('hidden', state.activeView !== 'players')
  nodes.factionsView.classList.toggle('hidden', state.activeView !== 'factions')
  nodes.permissionsView.classList.toggle('hidden', state.activeView !== 'permissions')
  document.querySelectorAll('.nav-button').forEach(button => {
    button.classList.toggle('active', button.dataset.view === state.activeView)
  })
}

function selectedPlayer() {
  return state.players.find(player => player.profileId === state.selectedProfileId) || null
}

function filteredPlayers() {
  const q = String(nodes.playerSearchInput.value || '').trim().toLowerCase()
  if (!q) return state.players
  return state.players.filter(player =>
    String(player.profileId).includes(q) ||
    String(player.discordId || '').toLowerCase().includes(q) ||
    String(player.username || '').toLowerCase().includes(q) ||
    String(player.displayName || '').toLowerCase().includes(q)
  )
}

function renderPlayers() {
  const rows = filteredPlayers()
  nodes.playerCount.textContent = `${rows.length} shown`
  nodes.playersTable.innerHTML = `
    <table>
      <thead>
        <tr>
          <th>Profile</th>
          <th>Name</th>
          <th>Discord ID</th>
          <th>Access</th>
          <th>Factions</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${rows.map(player => `
          <tr class="${player.profileId === state.selectedProfileId ? 'selected' : ''}">
            <td>#${player.profileId}</td>
            <td>${escapeHtml(player.displayName || player.username || 'Unnamed')}</td>
            <td>${escapeHtml(player.discordId)}</td>
            <td><span class="tag ${player.access?.allowed ? '' : 'locked'}">${escapeHtml(player.access?.allowed ? 'allowed' : (player.access?.error || 'blocked'))}</span></td>
            <td>${escapeHtml((player.assignments || []).map(a => a.requirement ? `${a.requirement.group} ${a.requirement.rank}` : a.requirementId).join(', '))}</td>
            <td><button class="ghost mini" data-select-player="${player.profileId}" type="button">Open</button></td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `
  renderPlayerDetail()
}

function renderPlayerDetail() {
  const player = selectedPlayer()
  nodes.selectedPlayerLabel.textContent = player ? `Profile #${player.profileId}` : 'New player'
  nodes.playerDiscordIdInput.value = player?.discordId || ''
  nodes.playerDiscordIdInput.disabled = !!player
  nodes.playerUsernameInput.value = player?.username || ''
  nodes.playerDisplayNameInput.value = player?.displayName || ''
  nodes.playerNotesInput.value = player?.notes || ''
  nodes.whitelistPlayerButton.disabled = !player
  nodes.banPlayerButton.disabled = !player
  nodes.whitelistPlayerButton.textContent = player?.access?.roles?.includes(state.access?.whitelistRoleId)
    ? 'Remove Whitelist'
    : 'Add Whitelist'
  nodes.banPlayerButton.textContent = player?.access?.roles?.includes(state.access?.bannedRoleId)
    ? 'Remove Ban'
    : 'Add Ban'
  nodes.playerAssignmentsTable.innerHTML = player
    ? renderPlayerAssignments(player)
    : '<div class="empty-row">Save the player before assigning factions.</div>'
  nodes.playerRequirementSelect.innerHTML = state.requirements
    .map(req => `<option value="${escapeHtml(req.id)}">${escapeHtml(req.group)} - ${escapeHtml(req.rank)}</option>`)
    .join('')
  nodes.playerFactionForm.querySelector('button[type="submit"]').disabled = !player
}

function renderPlayerAssignments(player) {
  const assignments = player.assignments || []
  if (!assignments.length) return '<div class="empty-row">No faction slots assigned.</div>'
  return `
    <table>
      <thead>
        <tr>
          <th>Group</th>
          <th>Rank</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${assignments.map(assignment => `
          <tr>
            <td>${escapeHtml(assignment.requirement?.group || assignment.requirementId)}</td>
            <td>${escapeHtml(assignment.requirement?.rank || '')}</td>
            <td><button class="danger mini" data-delete-player-assignment="${escapeHtml(assignment.id)}" type="button">Remove</button></td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `
}

function renderAccess() {
  const access = state.access || {}
  nodes.serverLockedInput.checked = access.serverLocked === true
  nodes.lockedRoleIdsInput.value = (access.lockedRoleIds || []).join('\n')
  nodes.lockedDiscordIdsInput.value = (access.lockedDiscordIds || []).join('\n')
  nodes.whitelistRoleIdInput.value = access.whitelistRoleId || ''
  nodes.bannedRoleIdInput.value = access.bannedRoleId || ''
}

function filteredRequirements() {
  const scope = nodes.scopeFilter.value
  const group = nodes.groupFilter.value
  return state.requirements.filter(req =>
    (!scope || req.scope === scope) &&
    (!group || req.group === group)
  )
}

function renderStats() {
  const uniqueSlots = state.requirements.filter(req => req.capacity === 1).length
  const openUnique = state.requirements.filter(req => req.capacity === 1 && req.assigned === 0).length
  const repeatable = state.requirements.filter(req => req.capacity === null).length
  nodes.stats.innerHTML = [
    ['Assignments', state.assignments.length],
    ['Unique Open', openUnique],
    ['Unique Slots', uniqueSlots],
    ['Repeatable Ranks', repeatable],
  ].map(([label, value]) => `
    <div class="stat">
      <strong>${value}</strong>
      <span class="muted">${label}</span>
    </div>
  `).join('')
}

function renderFilters() {
  const groups = [...new Set(state.requirements.map(req => req.group))].sort()
  const selected = nodes.groupFilter.value
  nodes.groupFilter.innerHTML = '<option value="">All</option>' + groups
    .map(group => `<option value="${escapeHtml(group)}">${escapeHtml(group)}</option>`)
    .join('')
  nodes.groupFilter.value = groups.includes(selected) ? selected : ''
}

function renderRequirements() {
  const rows = filteredRequirements()
  nodes.slotCount.textContent = `${rows.length} shown`
  nodes.requirementsTable.innerHTML = `
    <table>
      <thead>
        <tr>
          <th>Scope</th>
          <th>Group</th>
          <th>Rank</th>
          <th>Slots</th>
          <th>Permission</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${rows.map(req => `
          <tr class="${req.id === state.selectedRequirementId ? 'selected' : ''}">
            <td><span class="tag">${escapeHtml(req.scope)}</span></td>
            <td>${escapeHtml(req.group)}</td>
            <td>${escapeHtml(req.rank)}</td>
            <td>${req.capacity === null ? 'Open' : `${req.assigned}/${req.capacity}`}</td>
            <td>${escapeHtml(req.permission)}</td>
            <td><button class="ghost mini" data-select="${escapeHtml(req.id)}" type="button">Select</button></td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `

  nodes.requirementSelect.innerHTML = state.requirements
    .map(req => `<option value="${escapeHtml(req.id)}">${escapeHtml(req.group)} - ${escapeHtml(req.rank)}</option>`)
    .join('')

  if (!state.selectedRequirementId && state.requirements[0]) {
    state.selectedRequirementId = state.requirements[0].id
  }
  nodes.requirementSelect.value = state.selectedRequirementId
  renderSelectedSlot()
}

function renderSelectedSlot() {
  const req = state.requirements.find(item => item.id === state.selectedRequirementId)
  nodes.selectedSlot.textContent = req ? `${req.group} - ${req.rank}` : 'No slot selected'
}

function renderAssignments() {
  const byReq = new Map(state.requirements.map(req => [req.id, req]))
  nodes.assignmentCount.textContent = `${state.assignments.length} total`
  nodes.assignmentsTable.innerHTML = `
    <table>
      <thead>
        <tr>
          <th>Player</th>
          <th>Discord ID</th>
          <th>Group</th>
          <th>Rank</th>
          <th>Permission</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${state.assignments.map(assignment => {
          const req = byReq.get(assignment.requirementId) || {}
          return `
            <tr>
              <td>${escapeHtml(assignment.playerName || 'Unnamed')}</td>
              <td>${escapeHtml(assignment.discordId)}</td>
              <td>${escapeHtml(req.group || '')}</td>
              <td>${escapeHtml(req.rank || '')}</td>
              <td>${escapeHtml(req.permission || '')}</td>
              <td><button class="danger mini" data-delete-assignment="${escapeHtml(assignment.id)}" type="button">Remove</button></td>
            </tr>
          `
        }).join('')}
      </tbody>
    </table>
  `
}

function renderFactions() {
  renderStats()
  renderFilters()
  renderRequirements()
  renderAssignments()
}

function renderPermissionChecks(selected = []) {
  nodes.permissionChecks.innerHTML = state.knownPermissions.map(permission => `
    <label class="check-row">
      <input type="checkbox" value="${escapeHtml(permission)}" ${selected.includes(permission) ? 'checked' : ''}>
      <span>${escapeHtml(permission)}</span>
    </label>
  `).join('')
}

function renderRoles() {
  const entries = Object.entries(state.roles)
  nodes.roleCount.textContent = `${entries.length} configured`
  nodes.rolesTable.innerHTML = `
    <table>
      <thead>
        <tr>
          <th>Role</th>
          <th>Role ID</th>
          <th>Permissions</th>
          <th></th>
        </tr>
      </thead>
      <tbody>
        ${entries.map(([roleId, role]) => `
          <tr>
            <td>${escapeHtml(role.name)}</td>
            <td>${escapeHtml(roleId)}</td>
            <td>${escapeHtml((role.permissions || []).join(', '))}</td>
            <td>
              <button class="ghost mini" data-edit-role="${escapeHtml(roleId)}" type="button">Edit</button>
              <button class="danger mini" data-delete-role="${escapeHtml(roleId)}" type="button">Delete</button>
            </td>
          </tr>
        `).join('')}
      </tbody>
    </table>
  `
  renderPermissionChecks()
}

async function refreshAll() {
  await loadAccess()
  renderAccess()
  await loadFactions()
  renderFactions()
  await loadPlayers()
  renderPlayers()
  try {
    await loadPermissions()
    renderRoles()
  } catch (err) {
    nodes.permissionsView.innerHTML = `<section class="empty-state"><h2>Permission access unavailable</h2><p>${escapeHtml(err.message)}</p></section>`
  }
}

async function refreshPlayers() {
  await loadPlayers()
  renderPlayers()
}

function clearSelectedPlayer() {
  state.selectedProfileId = null
  nodes.playerForm.reset()
  nodes.playerFactionNotesInput.value = ''
  renderPlayers()
}

function lines(value) {
  return String(value || '')
    .split(/\r?\n|,/)
    .map(item => item.trim())
    .filter(Boolean)
}

async function saveAccess(event) {
  event.preventDefault()
  state.access = await api('/api/server-access', {
    method: 'PUT',
    body: JSON.stringify({
      serverLocked: nodes.serverLockedInput.checked,
      lockedRoleIds: lines(nodes.lockedRoleIdsInput.value),
      lockedDiscordIds: lines(nodes.lockedDiscordIdsInput.value),
      whitelistRoleId: nodes.whitelistRoleIdInput.value,
      bannedRoleId: nodes.bannedRoleIdInput.value,
    }),
  })
  renderAccess()
  toast('Server access policy saved')
}

async function checkAccess(event) {
  event.preventDefault()
  const discordId = nodes.accessCheckDiscordId.value.trim()
  const result = await api(`/api/server-access/check/${encodeURIComponent(discordId)}`)
  nodes.accessCheckResult.textContent = result.allowed
    ? `Allowed (${result.roles.length} role${result.roles.length === 1 ? '' : 's'})`
    : `Blocked: ${result.error || 'accessDenied'}`
}

async function savePlayer(event) {
  event.preventDefault()
  const player = selectedPlayer()
  const body = {
    discordId: nodes.playerDiscordIdInput.value,
    username: nodes.playerUsernameInput.value,
    displayName: nodes.playerDisplayNameInput.value,
    notes: nodes.playerNotesInput.value,
  }
  const saved = player
    ? await api(`/api/players/${player.profileId}`, { method: 'PUT', body: JSON.stringify(body) })
    : await api('/api/players', { method: 'POST', body: JSON.stringify(body) })
  state.selectedProfileId = saved.profileId
  await refreshPlayers()
  toast('Player saved')
}

async function toggleWhitelist() {
  const player = selectedPlayer()
  if (!player) return
  const enabled = !player.access?.roles?.includes(state.access?.whitelistRoleId)
  await api(`/api/players/${player.profileId}/whitelist`, {
    method: 'PUT',
    body: JSON.stringify({ enabled }),
  })
  await refreshPlayers()
  toast(enabled ? 'Player whitelisted' : 'Player removed from whitelist')
}

async function toggleBan() {
  const player = selectedPlayer()
  if (!player) return
  const enabled = !player.access?.roles?.includes(state.access?.bannedRoleId)
  await api(`/api/players/${player.profileId}/ban`, {
    method: 'PUT',
    body: JSON.stringify({ enabled }),
  })
  await refreshPlayers()
  toast(enabled ? 'Player banned' : 'Player unbanned')
}

async function assignSelectedPlayerFaction(event) {
  event.preventDefault()
  const player = selectedPlayer()
  if (!player) return
  await api(`/api/players/${player.profileId}/factions`, {
    method: 'POST',
    body: JSON.stringify({
      requirementId: nodes.playerRequirementSelect.value,
      playerName: player.displayName || player.username,
      notes: nodes.playerFactionNotesInput.value,
    }),
  })
  nodes.playerFactionNotesInput.value = ''
  await loadFactions()
  renderFactions()
  await refreshPlayers()
  toast('Faction assigned')
}

async function saveAssignment(event) {
  event.preventDefault()
  const body = {
    requirementId: nodes.requirementSelect.value,
    discordId: nodes.discordIdInput.value,
    playerName: nodes.playerNameInput.value,
    notes: nodes.notesInput.value,
  }
  await api('/api/faction-whitelist/assignments', {
    method: 'POST',
    body: JSON.stringify(body),
  })
  nodes.assignmentForm.reset()
  await loadFactions()
  renderFactions()
  toast('Assignment saved')
}

async function saveRole(event) {
  event.preventDefault()
  const permissions = [...nodes.permissionChecks.querySelectorAll('input:checked')].map(input => input.value)
  await api(`/api/role-permissions/${encodeURIComponent(nodes.roleIdInput.value)}`, {
    method: 'PUT',
    body: JSON.stringify({
      name: nodes.roleNameInput.value,
      permissions,
    }),
  })
  nodes.roleForm.reset()
  nodes.selectedRole.textContent = 'New role'
  await loadPermissions()
  renderRoles()
  toast('Role permissions saved')
}

function bindEvents() {
  nodes.loginButton.addEventListener('click', () => login().catch(err => toast(err.message)))
  nodes.logoutButton.addEventListener('click', () => logout())
  nodes.refreshButton.addEventListener('click', () => refreshAll().catch(err => toast(err.message)))
  nodes.newPlayerButton.addEventListener('click', clearSelectedPlayer)
  nodes.playersRefreshButton.addEventListener('click', () => refreshPlayers().catch(err => toast(err.message)))
  nodes.playerSearchInput.addEventListener('input', renderPlayers)
  nodes.playerForm.addEventListener('submit', event => savePlayer(event).catch(err => toast(err.message)))
  nodes.whitelistPlayerButton.addEventListener('click', () => toggleWhitelist().catch(err => toast(err.message)))
  nodes.banPlayerButton.addEventListener('click', () => toggleBan().catch(err => toast(err.message)))
  nodes.playerFactionForm.addEventListener('submit', event => assignSelectedPlayerFaction(event).catch(err => toast(err.message)))
  nodes.accessForm.addEventListener('submit', event => saveAccess(event).catch(err => toast(err.message)))
  nodes.accessCheckForm.addEventListener('submit', event => checkAccess(event).catch(err => toast(err.message)))
  nodes.scopeFilter.addEventListener('change', renderRequirements)
  nodes.groupFilter.addEventListener('change', renderRequirements)
  nodes.requirementSelect.addEventListener('change', event => {
    state.selectedRequirementId = event.target.value
    renderSelectedSlot()
    renderRequirements()
  })
  nodes.assignmentForm.addEventListener('submit', event => saveAssignment(event).catch(err => toast(err.message)))
  nodes.roleForm.addEventListener('submit', event => saveRole(event).catch(err => toast(err.message)))

  document.querySelector('.nav').addEventListener('click', event => {
    const button = event.target.closest('[data-view]')
    if (!button) return
    state.activeView = button.dataset.view
    renderActiveView()
  })

  document.body.addEventListener('click', event => {
    const select = event.target.closest('[data-select]')
    if (select) {
      state.selectedRequirementId = select.dataset.select
      nodes.requirementSelect.value = state.selectedRequirementId
      renderRequirements()
      return
    }

    const selectPlayer = event.target.closest('[data-select-player]')
    if (selectPlayer) {
      state.selectedProfileId = Number(selectPlayer.dataset.selectPlayer)
      renderPlayers()
      return
    }

    const deleteAssignment = event.target.closest('[data-delete-assignment]')
    if (deleteAssignment) {
      api(`/api/faction-whitelist/assignments/${encodeURIComponent(deleteAssignment.dataset.deleteAssignment)}`, {
        method: 'DELETE',
      })
        .then(loadFactions)
        .then(renderFactions)
        .then(refreshPlayers)
        .then(() => toast('Assignment removed'))
        .catch(err => toast(err.message))
      return
    }

    const deletePlayerAssignment = event.target.closest('[data-delete-player-assignment]')
    if (deletePlayerAssignment) {
      const player = selectedPlayer()
      if (!player) return
      api(`/api/players/${player.profileId}/factions/${encodeURIComponent(deletePlayerAssignment.dataset.deletePlayerAssignment)}`, {
        method: 'DELETE',
      })
        .then(loadFactions)
        .then(renderFactions)
        .then(refreshPlayers)
        .then(() => toast('Faction removed'))
        .catch(err => toast(err.message))
      return
    }

    const editRole = event.target.closest('[data-edit-role]')
    if (editRole) {
      const roleId = editRole.dataset.editRole
      const role = state.roles[roleId]
      nodes.roleIdInput.value = roleId
      nodes.roleNameInput.value = role.name || ''
      nodes.selectedRole.textContent = role.name || roleId
      renderPermissionChecks(role.permissions || [])
      return
    }

    const deleteRole = event.target.closest('[data-delete-role]')
    if (deleteRole) {
      api(`/api/role-permissions/${encodeURIComponent(deleteRole.dataset.deleteRole)}`, { method: 'DELETE' })
        .then(loadPermissions)
        .then(renderRoles)
        .then(() => toast('Role removed'))
        .catch(err => toast(err.message))
    }
  })
}

async function start() {
  bindEvents()
  captureTokenFromUrl()
  try {
    await loadSession()
    renderAuth()
    if (state.user) await refreshAll()
  } catch (err) {
    localStorage.removeItem(tokenKey)
    state.token = ''
    state.user = null
    renderAuth()
    toast(err.message)
  }

  const url = new URL(window.location.href)
  const error = url.searchParams.get('error')
  if (error) toast(error)
}

start()
