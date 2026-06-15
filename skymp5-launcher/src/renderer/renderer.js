// ── Window controls ──────────────────────────────────────────────────────────
document.getElementById('btn-minimize').addEventListener('click', () => window.electronAPI.minimize())
document.getElementById('btn-maximize').addEventListener('click', () => window.electronAPI.maximize())
document.getElementById('btn-close').addEventListener('click',    () => window.electronAPI.close())

// ── External nav links ────────────────────────────────────────────────────────
const EXTERNAL_URLS = {
  website: 'https://skyrimroleplay.co.uk/',   // e.g. 'https://skyrp.example.com'
  discord: 'https://discord.gg/xKY4Nud2rz',   // e.g. 'https://discord.gg/...'
}

document.querySelectorAll('.topnav-link[data-href]').forEach(link => {
  link.addEventListener('click', () => {
    const url = EXTERNAL_URLS[link.dataset.href]
    if (url) window.electronAPI.openExternal(url)
  })
})

// ── Settings modal ────────────────────────────────────────────────────────────
const modalOverlay = document.getElementById('modal-settings')

function openModal() { modalOverlay.hidden = false }
function closeModal() { modalOverlay.hidden = true }

document.getElementById('btn-gear').addEventListener('click', openModal)
document.getElementById('modal-close').addEventListener('click', closeModal)
modalOverlay.addEventListener('click', e => { if (e.target === modalOverlay) closeModal() })

// ── Settings tabs ─────────────────────────────────────────────────────────────
document.querySelectorAll('.modal-tab').forEach(tab => {
  tab.addEventListener('click', () => {
    document.querySelectorAll('.modal-tab').forEach(t => t.classList.remove('active'))
    document.querySelectorAll('.tab-panel').forEach(p => { p.hidden = true })
    tab.classList.add('active')
    document.getElementById(`tab-${tab.dataset.tab}`).hidden = false
  })
})

// ── Form fields ───────────────────────────────────────────────────────────────
const fieldSkyrimPath   = document.getElementById('setting-skyrim-path')

// ── Footer server selector ────────────────────────────────────────────────────
const footerServerName   = document.getElementById('footer-server-name')
const footerServerSelect = document.getElementById('footer-server-select')

footerServerSelect.addEventListener('change', () => {
  window.electronAPI.saveSettings({ activeServerIndex: parseInt(footerServerSelect.value, 10) })
})

// ── MO2 fields ────────────────────────────────────────────────────────────────
const fieldMo2Enabled = document.getElementById('setting-mo2-enabled')
const mo2StatusDot    = document.getElementById('mo2-status-dot')
const mo2StatusText   = document.getElementById('mo2-status-text')

// ── Discord auth state (kept in module scope for PLAY check) ──────────────────
let discordUser         = null
let discordAuthRequired = false
let serverLocked        = false
// Whether the current user is allowed to join (session-aware: set after login
// by re-fetching /api/serverinfo with X-Session).  Defaults true so unauthed
// users are not blocked before they have a chance to log in.
let serverAllowed       = true

// Re-evaluates Play button state whenever lock/whitelist state changes.
// Call this after login, logout, and initial serverinfo load.
function updateLockState() {
  // While the game runs (or a play sequence is in flight) the button is
  // managed by updatePlayButton() — don't fight over it here.
  if (gameRunning || playBusy) return

  if (serverLocked && discordUser && !serverAllowed) {
    // Logged in but not on the server lock allow-list
    btnConnect.disabled = true
    btnConnect.title    = 'The server is currently locked.'
    connectWarning.textContent = 'Server is currently locked — you are not on the allow list.'
    connectWarning.classList.add('visible')
  } else if (!serverLocked && discordUser && !serverAllowed) {
    // Logged in but not on the whitelist
    btnConnect.disabled = true
    btnConnect.title    = 'You are not on the server whitelist.'
    connectWarning.textContent = 'You are not on the server whitelist.'
    connectWarning.classList.add('visible')
  } else {
    btnConnect.disabled = false
    btnConnect.title    = ''
    // Fix instantly disappearing
    const lockMessages = [
      'Server is currently locked — you are not on the whitelist.',
      'You are not on the server whitelist.',
    ]
    if (lockMessages.includes(connectWarning.textContent)) {
      connectWarning.classList.remove('visible')
      connectWarning.textContent = ''
    }
  }
}

// ── Load / save settings ──────────────────────────────────────────────────────
async function loadSettings() {
  const s = await window.electronAPI.loadSettings()
  fieldSkyrimPath.value = s.skyrimPath || ''

  // Footer server selector — dropdown when >1 server, plain text otherwise
  if (s.servers && s.servers.length > 1) {
    footerServerName.hidden   = true
    footerServerSelect.hidden = false
    footerServerSelect.innerHTML = ''
    s.servers.forEach((srv, i) => {
      const opt = document.createElement('option')
      opt.value       = i
      opt.textContent = srv.name
      opt.selected    = i === (s.activeServerIndex || 0)
      footerServerSelect.appendChild(opt)
    })
  } else {
    footerServerName.hidden   = false
    footerServerSelect.hidden = true
    if (s.servers && s.servers.length === 1) {
      footerServerName.textContent = s.servers[0].name
    }
  }

  // Restore Discord user from persisted store
  if (s.discordUser) {
    discordUser = s.discordUser
    renderTopbarDiscord()
  }

  // Restore MO2 settings
  fieldMo2Enabled.checked = !!s.mo2Enabled
  refreshMo2Status()

  // Restore isolated-game setting
  fieldIsolated.checked = !!s.isolatedGame
  refreshIsolatedStatus()

  return s
}

// ── Discord topbar widget ─────────────────────────────────────────────────────
const discordTopbarSlot = document.getElementById('discord-topbar-slot')

function renderTopbarDiscord() {
  discordTopbarSlot.innerHTML = ''

  if (discordUser) {
    const wrap = document.createElement('div')
    wrap.className = 'discord-topbar-user'

    if (discordUser.avatar) {
      const img = document.createElement('img')
      img.className = 'discord-topbar-avatar'
      img.src = discordUser.avatar
      img.alt = discordUser.username
      wrap.appendChild(img)
    } else {
      const ph = document.createElement('div')
      ph.className   = 'discord-topbar-avatar-placeholder'
      ph.textContent = '✦'
      wrap.appendChild(ph)
    }

    const name = document.createElement('span')
    name.className   = 'discord-topbar-name'
    name.textContent = `Discord: ${discordUser.tag || discordUser.username}`
    wrap.appendChild(name)

    const logoutBtn = document.createElement('button')
    logoutBtn.className   = 'discord-topbar-logout'
    logoutBtn.title       = 'Logout'
    logoutBtn.textContent = '✕'
    logoutBtn.addEventListener('click', async () => {
      await window.electronAPI.discordLogout()
      discordUser   = null
      serverAllowed = true  // reset: access unknown until next login
      renderTopbarDiscord()
      updateLockState()
    })
    wrap.appendChild(logoutBtn)

    discordTopbarSlot.appendChild(wrap)
  } else {
    const loginBtn = document.createElement('button')
    loginBtn.className   = 'btn-discord-topbar'
    loginBtn.textContent = 'Discord Login'
    loginBtn.addEventListener('click', async () => {
      loginBtn.disabled    = true
      loginBtn.textContent = 'Opening…'
      const result = await window.electronAPI.discordLogin()
      if (result.success) {
        discordUser = result.user
        // Re-fetch serverinfo now that we have a session — the backend will
        // evaluate whitelist / lock access and return the correct `allowed` flag.
        const freshInfo = await window.electronAPI.fetchServerInfo()
        serverAllowed = freshInfo ? freshInfo.allowed !== false : true
        renderTopbarDiscord()
        updateLockState()
      } else {
        loginBtn.disabled    = false
        loginBtn.textContent = 'Discord Login'
        connectWarning.textContent = `Discord: ${result.error}`
        connectWarning.classList.add('visible')
        setTimeout(() => connectWarning.classList.remove('visible'), 4000)
      }
    })
    discordTopbarSlot.appendChild(loginBtn)
  }
}

renderTopbarDiscord()


// ── Nexus topbar widget ───────────────────────────────────────────────────────
const nexusTopbarSlot  = document.getElementById('nexus-topbar-slot')
const modalNexus       = document.getElementById('modal-nexus')
const nexusKeyInput    = document.getElementById('nexus-key-input')
const nexusLoginStatus = document.getElementById('nexus-login-status')

let nexusUser = null

function renderTopbarNexus() {
  nexusTopbarSlot.innerHTML = ''

  if (nexusUser) {
    const wrap = document.createElement('div')
    wrap.className = 'discord-topbar-user nexus-topbar-user'

    if (nexusUser.profileUrl) {
      const img = document.createElement('img')
      img.className = 'discord-topbar-avatar'
      img.src = nexusUser.profileUrl
      img.alt = nexusUser.name
      wrap.appendChild(img)
    }

    const name = document.createElement('span')
    name.className   = 'discord-topbar-name'
    name.textContent = `Nexus: ${nexusUser.name}${nexusUser.isPremium ? ' \u2605' : ''}`
    name.title       = nexusUser.isPremium
      ? 'Nexus Premium — automatic mod downloads enabled'
      : 'Nexus free account — downloads open in the browser'
    wrap.appendChild(name)

    const logoutBtn = document.createElement('button')
    logoutBtn.className   = 'discord-topbar-logout'
    logoutBtn.title       = 'Logout from Nexus'
    logoutBtn.textContent = '\u2715'
    logoutBtn.addEventListener('click', async () => {
      await window.electronAPI.nexusLogout()
      nexusUser = null
      renderTopbarNexus()
    })
    wrap.appendChild(logoutBtn)

    nexusTopbarSlot.appendChild(wrap)
  } else {
    const loginBtn = document.createElement('button')
    loginBtn.className   = 'btn-nexus-topbar'
    loginBtn.textContent = 'Nexus Login'
    loginBtn.addEventListener('click', () => {
      nexusLoginStatus.textContent = ''
      nexusKeyInput.value = ''
      modalNexus.hidden = false
      nexusKeyInput.focus()
    })
    nexusTopbarSlot.appendChild(loginBtn)
  }
}

// Show the one-click SSO button only when the app has a registered slug
window.electronAPI.nexusSsoAvailable().then(available => {
  document.getElementById('nexus-sso-group').hidden = !available
})

document.getElementById('nexus-sso-login').addEventListener('click', async () => {
  const btn = document.getElementById('nexus-sso-login')
  btn.disabled = true
  nexusLoginStatus.textContent = 'Waiting for you to authorise in the browser…'

  const result = await window.electronAPI.nexusSsoLogin()
  btn.disabled = false

  if (!result.success) {
    nexusLoginStatus.textContent = `Error: ${result.error}`
    return
  }

  nexusUser = result.user
  nexusLoginStatus.textContent = ''
  modalNexus.hidden = true
  renderTopbarNexus()
})

document.getElementById('nexus-modal-close').addEventListener('click', () => { modalNexus.hidden = true })
modalNexus.addEventListener('click', e => { if (e.target === modalNexus) modalNexus.hidden = true })

document.getElementById('nexus-keys-link').addEventListener('click', e => {
  e.preventDefault()
  window.electronAPI.openExternal('https://next.nexusmods.com/settings/api-keys')
})

document.getElementById('nexus-key-save').addEventListener('click', async () => {
  const btn = document.getElementById('nexus-key-save')
  btn.disabled = true
  nexusLoginStatus.textContent = 'Validating key…'

  const result = await window.electronAPI.nexusLogin(nexusKeyInput.value)
  btn.disabled = false

  if (!result.success) {
    nexusLoginStatus.textContent = `Error: ${result.error}`
    return
  }

  nexusUser = result.user
  nexusLoginStatus.textContent = ''
  modalNexus.hidden = true
  renderTopbarNexus()
})

window.electronAPI.nexusGetUser().then(user => {
  nexusUser = user
  renderTopbarNexus()
})

// ── Isolated game copy UI ─────────────────────────────────────────────────────
const isolatedDot       = document.getElementById('isolated-status-dot')
const isolatedText      = document.getElementById('isolated-status-text')
const fieldIsolated     = document.getElementById('setting-isolated-game')
const btnCreateIsolated = document.getElementById('btn-create-isolated')

async function refreshIsolatedStatus() {
  const st = await window.electronAPI.isolatedStatus()
  if (!st.ready) {
    isolatedDot.className    = 'vortex-status-dot'
    isolatedText.textContent = 'Not installed yet — choose a location to set up SkyRP'
  } else if (!fieldIsolated.checked) {
    isolatedDot.className    = 'vortex-status-dot dot-warn'
    isolatedText.textContent = 'SkyRP install exists — playing from the original Skyrim'
  } else {
    isolatedDot.className    = 'vortex-status-dot dot-ok'
    isolatedText.textContent = `SkyRP installed at ${st.base || st.dir}`
  }
}

btnCreateIsolated.addEventListener('click', async () => {
  btnCreateIsolated.disabled = true
  btnCreateIsolated.textContent = 'Copying…'

  window.electronAPI.removeIsolatedListeners()
  // Game-copy steps run in the bottom install-status field; the line next to
  // this button stays on the not-installed/installed status only.
  window.electronAPI.onIsolatedProgress(msg => {
    installStatusMo2.textContent = msg
  })

  const result = await window.electronAPI.createIsolated()
  window.electronAPI.removeIsolatedListeners()

  btnCreateIsolated.disabled = false
  btnCreateIsolated.textContent = 'Choose location & install…'

  if (!result.success) {
    installStatusMo2.textContent = `Error: ${result.error}`
    return
  }
  fieldIsolated.checked = true
  await window.electronAPI.saveSettings({ isolatedGame: true })
  refreshIsolatedStatus()
  refreshPlayState()

  if (confirm('SkyRP game copy is ready. Download and install the modpack now?')) {
    startModpackInstall()
  }
})

fieldIsolated.addEventListener('change', refreshIsolatedStatus)

document.getElementById('btn-save').addEventListener('click', async () => {
  const data = {
    skyrimPath:   fieldSkyrimPath.value.trim(),
    mo2Enabled:   fieldMo2Enabled.checked,
    isolatedGame: fieldIsolated.checked,
  }

  await window.electronAPI.saveSettings(data)
  refreshMo2Status()

  const btn = document.getElementById('btn-save')
  btn.textContent = 'Saved!'
  setTimeout(() => { btn.textContent = 'Save Settings' }, 1400)
})

// ── Browse folder ─────────────────────────────────────────────────────────────
document.getElementById('btn-browse').addEventListener('click', async () => {
  const folder = await window.electronAPI.openFolder()
  if (folder) fieldSkyrimPath.value = folder
})

// ── MO2 UI ────────────────────────────────────────────────────────────────────

const mo2EnableText = document.getElementById('mo2-enable-text')

async function refreshMo2Status() {
  const status  = await window.electronAPI.mo2Status()
  const enabled = fieldMo2Enabled.checked

  // Checkbox caption reflects what disabling MO2 means.
  mo2EnableText.textContent = enabled
    ? 'Launch the game through MO2 — mods stay out of your Skyrim folder'
    : 'You will need to install mods manually.'

  if (!status.installed) {
    mo2StatusDot.className    = 'vortex-status-dot'
    mo2StatusText.textContent = 'MO2 not installed yet — run "Install Modpack via MO2" below'
  } else if (!enabled) {
    mo2StatusDot.className    = 'vortex-status-dot dot-warn'
    mo2StatusText.textContent = `MO2 ${status.version} ready (${status.modCount} mods) — launching without it`
  } else {
    mo2StatusDot.className    = 'vortex-status-dot dot-ok'
    mo2StatusText.textContent = `MO2 ${status.version} active (${status.modCount} mods)`
  }
}

const btnOpenMo2  = document.getElementById('btn-open-mo2')
const mo2OpenWarn = document.getElementById('mo2-open-warning')
btnOpenMo2.addEventListener('click', async () => {
  btnOpenMo2.disabled    = true
  btnOpenMo2.textContent = 'MO2 is running'
  if (mo2OpenWarn) mo2OpenWarn.hidden = false

  const result = await window.electronAPI.mo2Open()
  if (!result.success) {
    alert(`Could not open MO2: ${result.error}`)
    btnOpenMo2.disabled    = false
    btnOpenMo2.textContent = 'Open & Configure MO2'
    if (mo2OpenWarn) mo2OpenWarn.hidden = true
  }
})

fieldMo2Enabled.addEventListener('change', refreshMo2Status)

document.getElementById('btn-open-install').addEventListener('click', async () => {
  const r = await window.electronAPI.openInstallFolder()
  if (!r.success) alert(`Could not open the install folder: ${r.error}`)
})

// ── Troubleshooting: manual launch buttons ───────────────────────────────────
const troubleLaunchStatus = document.getElementById('trouble-launch-status')

document.getElementById('btn-launch-mo2').addEventListener('click', async () => {
  troubleLaunchStatus.textContent = 'Launching via MO2…'
  const r = await window.electronAPI.launchViaMO2()
  troubleLaunchStatus.textContent = r.success ? 'Launched via MO2 ✓' : `Error: ${r.error}`
})

document.getElementById('btn-launch-direct').addEventListener('click', async () => {
  troubleLaunchStatus.textContent = 'Launching SKSE…'
  const r = await window.electronAPI.launchDirect()
  troubleLaunchStatus.textContent = r.success ? 'Launched ✓' : `Error: ${r.error}`
})

// ── Install / Update Client Files ────────────────────────────────────────────
const installStatusClient = document.getElementById('install-status-client')

document.getElementById('btn-install-client').addEventListener('click', () => {
  installStatusClient.textContent = 'Starting install…'
  window.electronAPI.removeInstallListeners()

  window.electronAPI.onInstallProgress(({ phase, file, index, total, skipped }) => {
    if (phase === 'download') {
      installStatusClient.textContent = file
    } else {
      const prefix = skipped ? '[skip]' : `[${index}/${total}]`
      installStatusClient.textContent = `${prefix} ${file}`
    }
  })

  window.electronAPI.onInstallComplete(({ success, error, upToDate }) => {
    if (!success) {
      installStatusClient.textContent = `Error: ${error}`
      return
    }
    installStatusClient.textContent = upToDate ? 'Client files up to date ✓' : 'Client files installed ✓'
  })

  window.electronAPI.startInstall('client')
})

// ── Install Modpack via MO2 ───────────────────────────────────────────────────
const installStatusMo2 = document.getElementById('install-status-mo2')

function startModpackInstall() {
  installStatusMo2.textContent = 'Starting MO2 install…'
  window.electronAPI.removeInstallListeners()

  window.electronAPI.onInstallProgress(({ phase, file, index, total, skipped }) => {
    if (phase === 'download') {
      installStatusMo2.textContent = file
    } else if (phase === 'mods') {
      installStatusMo2.textContent = total > 0 ? `[mods ${index}/${total}] ${file}` : file
    } else {
      const prefix = skipped ? '[skip]' : `[${index}/${total}]`
      installStatusMo2.textContent = `${prefix} ${file}`
    }
  })

  window.electronAPI.onInstallComplete(({ success, error, upToDate, warning, modsTotal }) => {
    if (!success) {
      installStatusMo2.textContent = `Error: ${error}`
      return
    }
    if (warning) {
      installStatusMo2.textContent = `⚠ ${warning}`
      refreshMo2Status()
      return
    }
    const files = upToDate ? 'client files up to date' : 'client files installed'
    installStatusMo2.textContent = `Modpack ready ✓ — ${modsTotal ?? 0} mods, ${files}`
    refreshMo2Status()
  })

  window.electronAPI.startInstall('mo2')
}
document.getElementById('btn-install-mo2').addEventListener('click', startModpackInstall)

// ── PLAY button ───────────────────────────────────────────────────────────────
// One click does everything: verify/refresh client files, sync the load
// order, then launch. While the game runs the button reflects that state.
const btnConnect     = document.getElementById('btn-connect')
const connectWarning = document.getElementById('connect-warning')

let gameRunning     = false
let playBusy        = false
let isoReady        = true   // isolation disabled, or the game copy exists
let updateAvailable = false  // server has newer client files than installed

const PLAY_LABEL = '\u25BA PLAY'
const updatePill = document.getElementById('update-pill')

function updatePlayButton() {
  updatePill.hidden = !(updateAvailable && isoReady && !gameRunning)

  if (gameRunning) {
    btnConnect.disabled    = true
    btnConnect.textContent = '\u23F3 GAME RUNNING'
    btnConnect.title       = 'Skyrim is currently running.'
    return
  }
  if (playBusy) return  // label managed by the play/update sequence

  if (!isoReady) {
    btnConnect.disabled    = false
    btnConnect.textContent = '\u2699 INSTALL'
    btnConnect.title       = 'Set up your SkyRP game copy in Settings.'
    return
  }

  if (updateAvailable) {
    btnConnect.disabled    = false
    btnConnect.textContent = '\u2913 UPDATE'
    btnConnect.title       = 'A client files update is available.'
    return
  }

  btnConnect.textContent = PLAY_LABEL
  btnConnect.title       = ''
  btnConnect.disabled    = false
  updateLockState()
}

// Re-evaluate the install/update state (called at startup, after installs,
// after the game copy is created, and on a slow poll).
async function refreshPlayState() {
  const iso = await window.electronAPI.isolatedStatus()
  isoReady = !iso.enabled || iso.ready

  const uc = await window.electronAPI.filesUpdateCheck()
  updateAvailable = !!uc.updateAvailable
  if (uc.serverVersion) clientVersionEl.textContent = `v${uc.serverVersion}`

  updatePlayButton()
}
setInterval(refreshPlayState, 60_000)

async function pollGameRunning() {
  const running = await window.electronAPI.gameIsRunning()
  if (running !== gameRunning) {
    gameRunning = running
    updatePlayButton()
  }
}
setInterval(pollGameRunning, 10_000)
pollGameRunning()

function showWarning(text) {
  connectWarning.textContent = text
  connectWarning.classList.add('visible')
}

function clearWarning() {
  connectWarning.classList.remove('visible')
  connectWarning.textContent = ''
}

// Run the installer (auto mode) and resolve with its completion result,
// mirroring progress onto the Play button / warning strip.
function runInstallForPlay() {
  return new Promise(resolve => {
    window.electronAPI.removeInstallListeners()
    window.electronAPI.onInstallProgress(({ phase, file }) => {
      btnConnect.textContent = phase === 'download' ? '\u2913 DOWNLOADING\u2026' : '\u2699 INSTALLING\u2026'
      showWarning(file)
    })
    window.electronAPI.onInstallComplete(result => resolve(result))
    window.electronAPI.startInstall('auto')
  })
}

btnConnect.addEventListener('click', async () => {
  if (gameRunning || playBusy) return

  // No game copy yet: the button reads INSTALL and leads to Settings,
  // where the isolated-copy setup lives.
  if (!isoReady) {
    openModal()
    return
  }

  // Update mode: refresh the client files, don't launch.
  if (updateAvailable) {
    playBusy            = true
    btnConnect.disabled = true
    clearWarning()
    try {
      btnConnect.textContent = '\u2913 UPDATING\u2026'
      const result = await runInstallForPlay()
      if (!result.success) {
        showWarning(result.error || 'Update failed.')
        return
      }
      showWarning('Client files updated \u2713')
      setTimeout(clearWarning, 4000)
    } finally {
      playBusy = false
      await refreshPlayState()
    }
    return
  }

  if (discordUser && !serverAllowed) {
    showWarning(serverLocked
      ? 'Server is currently locked — you are not on the allow list.'
      : 'You are not on the server whitelist.')
    return
  }

  const s = await window.electronAPI.loadSettings()
  if (!s.skyrimPath) {
    showWarning('Set Skyrim path in Settings first.')
    return
  }

  if (!discordUser) {
    showWarning('Login with Discord first — use the button in the toolbar.')
    return
  }

  playBusy            = true
  btnConnect.disabled = true
  clearWarning()

  try {
    // 1. Make sure client files are present and current (fast no-op when up to date)
    btnConnect.textContent = '\u2699 CHECKING FILES\u2026'
    const install = await runInstallForPlay()
    if (!install.success) {
      showWarning(install.error || 'Install failed.')
      return
    }

    // 2. Launch — main also re-syncs plugins.txt against the server load order
    btnConnect.textContent = '\u25BA LAUNCHING\u2026'
    clearWarning()
    const result = await window.electronAPI.launchSkse()

    if (!result.success) {
      showWarning(result.error)
      return
    }

    clearWarning()
    gameRunning = true  // optimistic; the 10s poll keeps it honest
  } finally {
    playBusy = false
    await refreshPlayState()
  }
})

// ── Server status ─────────────────────────────────────────────────────────────
const badgeStatus  = document.getElementById('badge-status')
const badgeLabel   = document.getElementById('badge-label')
const badgePlayers = document.getElementById('badge-players')
const footerPlayers = document.getElementById('footer-players')

async function checkServerStatus() {
  const data = await window.electronAPI.fetchStatus()
  if (!data || !data.ok || data.status !== 'online') {
    badgeStatus.classList.remove('online')
    badgeLabel.textContent = 'OFFLINE'
    badgePlayers.hidden = true
    footerPlayers.hidden = true
  } else {
    badgeStatus.classList.add('online')
    badgeLabel.textContent = 'ONLINE'
    if (data.players != null) {
      badgePlayers.textContent = `${data.players} PLAYERS`
      badgePlayers.hidden = false
      footerPlayers.textContent = `${data.players} online`
      footerPlayers.hidden = false
    } else {
      badgePlayers.hidden = true
      footerPlayers.hidden = true
    }
  }
}

// ── Server info strip ─────────────────────────────────────────────────────────
async function loadServerInfo() {
  const info = await window.electronAPI.fetchServerInfo()
  if (!info || info.error) return

  const strip      = document.getElementById('server-info-strip')
  const nameEl     = document.getElementById('sinfo-name')
  const capEl      = document.getElementById('sinfo-capacity')
  const modeEl     = document.getElementById('sinfo-mode')
  const modeSep    = document.getElementById('sinfo-mode-sep')
  const discEl     = document.getElementById('sinfo-discord')
  const discSep    = document.getElementById('sinfo-discord-sep')
  const lockEl     = document.getElementById('sinfo-locked')
  const lockSep    = document.getElementById('sinfo-locked-sep')
  const footerName = document.getElementById('footer-server-name')

  nameEl.textContent = info.name
  capEl.textContent  = `Max ${info.maxPlayers} players`
  footerName.textContent = info.name

  if (info.gamemode) {
    modeEl.textContent = info.gamemode
    modeEl.hidden  = false
    modeSep.hidden = false
  }

  if (info.discordAuthRequired) {
    discordAuthRequired = true
    discEl.hidden  = false
    discSep.hidden = false
  }

  if (info.locked) {
    serverLocked   = true
    lockEl.hidden  = false
    lockSep.hidden = false
  }

  // `allowed` is session-aware: false only when a session was sent and the
  // backend rejected it (locked/not whitelisted).  Without a session it
  // defaults to true — access is re-checked after Discord login.
  // `sessionValid: false` means the stored session expired — treat as logged out.
  if (info.sessionValid === false && discordUser) {
    // Session expired — clear stale auth so the user can log in again cleanly.
    await window.electronAPI.discordLogout()
    discordUser   = null
    serverAllowed = true
    renderTopbarDiscord()
  } else if (info.allowed === false) {
    serverAllowed = false
  }

  updateLockState()

  strip.hidden = false
}

// ── Launcher update check ─────────────────────────────────────────────────────
const launcherVersionEl = document.getElementById('launcher-version')
const clientVersionEl   = document.getElementById('client-version')

async function checkLauncherUpdate() {
  const result = await window.electronAPI.checkUpdate()
  if (!result) return

  if (result.hasUpdate) {
    launcherVersionEl.textContent = '⬆ UPDATE AVAILABLE'
    launcherVersionEl.classList.add('update-available')
    launcherVersionEl.title = `v${result.latest} is available — click to download`
    launcherVersionEl.addEventListener('click', () => {
      if (result.downloadUrl) window.electronAPI.openExternal(result.downloadUrl)
    })
  } else {
    launcherVersionEl.textContent = `v${result.current}`
    launcherVersionEl.classList.remove('update-available')
  }
}

// ── News ──────────────────────────────────────────────────────────────────────
const newsGrid = document.getElementById('news-grid')

// Shared error-state card with a retry button — used by news and modlist
// instead of silently showing fallback content when the backend is unreachable.
function buildErrorState(message, onRetry) {
  const box = document.createElement('div')
  box.className = 'panel-error'

  const text = document.createElement('div')
  text.className   = 'panel-error-text'
  text.textContent = message
  box.appendChild(text)

  const retry = document.createElement('button')
  retry.className   = 'panel-error-retry'
  retry.textContent = 'Retry'
  retry.addEventListener('click', () => {
    retry.disabled    = true
    retry.textContent = 'Retrying…'
    onRetry()
  })
  box.appendChild(retry)

  return box
}

function buildNewsCard(item) {
  const card = document.createElement('div')
  card.className = 'news-card'

  const imgWrap = document.createElement('div')
  imgWrap.className = 'news-card-image'
  if (item.image) {
    const img = document.createElement('img')
    img.src = item.image
    img.alt = item.title
    imgWrap.appendChild(img)
  }

  const body = document.createElement('div')
  body.className = 'news-card-body'

  const tag = document.createElement('div')
  tag.className = 'news-card-tag'
  tag.textContent = item.tag || 'UPDATE'

  const title = document.createElement('div')
  title.className = 'news-card-title'
  title.textContent = item.title

  const date = document.createElement('div')
  date.className = 'news-card-date'
  date.textContent = item.date

  body.appendChild(tag)
  body.appendChild(title)

  if (item.body) {
    const desc = document.createElement('div')
    desc.className = 'news-card-desc'
    desc.textContent = item.body
    body.appendChild(desc)
  }

  body.appendChild(date)

  card.appendChild(imgWrap)
  card.appendChild(body)
  return card
}

async function loadNews() {
  const result = await window.electronAPI.fetchNews()
  newsGrid.innerHTML = ''

  if (!result || !result.ok) {
    newsGrid.appendChild(buildErrorState('Couldn’t reach the server — news unavailable.', loadNews))
    return
  }

  if (result.items.length === 0) {
    const empty = document.createElement('div')
    empty.className   = 'panel-empty'
    empty.textContent = 'No news posted yet.'
    newsGrid.appendChild(empty)
    return
  }

  result.items.forEach(item => newsGrid.appendChild(buildNewsCard(item)))
}

// ── Modlist ───────────────────────────────────────────────────────────────────

const NEXUS_BASE = 'https://www.nexusmods.com/skyrimspecialedition/mods'

function buildModItem(mod) {
  const item = document.createElement('div')
  item.className = `modlist-item${mod.enabled ? '' : ' modlist-item--disabled'}`

  const dot = document.createElement('span')
  dot.className = `mod-dot ${mod.enabled ? 'mod-dot--enabled' : 'mod-dot--disabled'}`

  const name = document.createElement('span')
  name.className   = 'mod-name'
  name.textContent = mod.name
  name.title       = mod.name

  item.appendChild(dot)
  item.appendChild(name)

  if (mod.required) {
    const badge = document.createElement('span')
    badge.className   = 'mod-badge mod-badge--required'
    badge.textContent = 'REQ'
    item.appendChild(badge)
  }

  // Backend mods are installed automatically by the launcher.
  // Nexus mods are downloaded from Nexus and installed through MO2.
  if (mod.source === 'backend') {
    const badge = document.createElement('span')
    badge.className   = 'mod-badge mod-badge--auto'
    badge.textContent = 'AUTO'
    badge.title       = 'Installed automatically by the launcher'
    item.appendChild(badge)
  } else if (mod.source === 'nexus' && mod.nexusId) {
    const link = document.createElement('a')
    link.className   = 'mod-nexus-link'
    link.textContent = 'Nexus'
    link.title       = 'Open on Nexus Mods'
    link.href        = '#'
    link.addEventListener('click', e => {
      e.preventDefault()
      window.electronAPI.openExternal(`${NEXUS_BASE}/${mod.nexusId}`)
    })
    item.appendChild(link)
  }

  if (mod.version) {
    const ver = document.createElement('span')
    ver.className   = 'mod-version'
    ver.textContent = `v${mod.version}`
    item.appendChild(ver)
  }

  return item
}

// Keep a reference to the last-loaded modlist so the install handler can use it.
let currentModlist = []

async function loadModlist() {
  const panel = document.getElementById('modlist')
  const count = document.getElementById('modlist-count')

  const result = await window.electronAPI.fetchModlist()
  panel.innerHTML = ''

  if (!result || !result.ok) {
    currentModlist    = []
    count.textContent = '—'
    panel.appendChild(buildErrorState('Couldn’t reach the server — modlist unavailable.', loadModlist))
    return
  }

  currentModlist = result.items

  if (currentModlist.length === 0) {
    count.textContent = '0 mods'
    const empty = document.createElement('div')
    empty.className   = 'panel-empty'
    empty.textContent = 'No mods published yet.'
    panel.appendChild(empty)
    return
  }

  currentModlist.forEach(mod => panel.appendChild(buildModItem(mod)))

  const enabled = currentModlist.filter(m => m.enabled).length
  count.textContent = `${enabled} / ${currentModlist.length} enabled`
}

// ── Metrics modal ─────────────────────────────────────────────────────────────
const modalMetrics  = document.getElementById('modal-metrics')
const metricsGrid   = document.getElementById('metrics-grid')
const metricsLoading = document.getElementById('metrics-loading')

document.getElementById('btn-stats').addEventListener('click', () => {
  modalMetrics.hidden = false
  loadMetrics()
})

document.getElementById('metrics-close').addEventListener('click', () => {
  modalMetrics.hidden = true
})

modalMetrics.addEventListener('click', e => {
  if (e.target === modalMetrics) modalMetrics.hidden = true
})

function metricCard(label, value, sub) {
  const card = document.createElement('div')
  card.className = 'metric-card'

  const lEl = document.createElement('div')
  lEl.className   = 'metric-label'
  lEl.textContent = label

  const vEl = document.createElement('div')
  vEl.className   = 'metric-value'
  vEl.textContent = value

  card.appendChild(lEl)
  card.appendChild(vEl)

  if (sub != null) {
    const sEl = document.createElement('div')
    sEl.className   = 'metric-sub'
    sEl.textContent = sub
    card.appendChild(sEl)
  }

  return card
}

async function loadMetrics() {
  metricsGrid.innerHTML = ''
  const loadEl = document.createElement('div')
  loadEl.className   = 'metrics-loading'
  loadEl.textContent = 'Loading…'
  metricsGrid.appendChild(loadEl)

  const result = await window.electronAPI.fetchMetrics()

  metricsGrid.innerHTML = ''

  if (!result || !result.ok) {
    const err = document.createElement('div')
    err.className   = 'metric-card metric-card--error'
    err.textContent = 'Server statistics are currently unavailable.'
    if (result?.error) err.title = result.error
    metricsGrid.appendChild(err)
    return
  }

  const m = result.metrics

  const connects    = m['skymp_connects_total']    ?? null
  const disconnects = m['skymp_disconnects_total'] ?? null
  const online      = (connects !== null && disconnects !== null)
    ? Math.max(0, connects - disconnects)
    : null

  const logins      = m['skymp_logins_total']       ?? null
  const loginErrors = m['skymp_login_errors_total'] ?? null
  const rpcs        = m['skymp_rpc_calls_total']    ?? null
  const tickAvg     = m['skymp_tick_duration_seconds_sum'] != null && m['skymp_tick_duration_seconds_count']
    ? (m['skymp_tick_duration_seconds_sum'] / m['skymp_tick_duration_seconds_count'] * 1000)
    : null

  const fmt = v => v != null ? v.toLocaleString() : '—'
  const fmtMs = v => v != null ? `${v.toFixed(1)} ms` : '—'

  metricsGrid.appendChild(metricCard('Online Now',       fmt(online),      online !== null ? `${fmt(connects)} connects / ${fmt(disconnects)} disconnects` : null))
  metricsGrid.appendChild(metricCard('Total Logins',     fmt(logins),      loginErrors !== null ? `${fmt(loginErrors)} errors` : null))
  metricsGrid.appendChild(metricCard('RPC Calls',        fmt(rpcs),        null))
  metricsGrid.appendChild(metricCard('Avg Tick Duration', fmtMs(tickAvg),  null))
}

// ── Init ──────────────────────────────────────────────────────────────────────
loadSettings()
checkServerStatus()
checkLauncherUpdate()
loadNews()
loadServerInfo()
loadModlist()
setInterval(checkServerStatus, 30_000)
refreshPlayState()
