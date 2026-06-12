const { contextBridge, ipcRenderer } = require('electron')

contextBridge.exposeInMainWorld('electronAPI', {
  // Window controls
  minimize: () => ipcRenderer.send('window:minimize'),
  maximize: () => ipcRenderer.send('window:maximize'),
  close:    () => ipcRenderer.send('window:close'),

  // User settings (skyrimPath, username, activeServerIndex)
  loadSettings: ()     => ipcRenderer.invoke('settings:load'),
  saveSettings: (data) => ipcRenderer.invoke('settings:save', data),

  // OS folder picker
  openFolder: () => ipcRenderer.invoke('dialog:openFolder'),

  // API calls proxied through main (keeps CSP clean, uses config.js values)
  fetchStatus:     () => ipcRenderer.invoke('api:status'),
  fetchNews:       () => ipcRenderer.invoke('api:news'),
  fetchServerInfo: () => ipcRenderer.invoke('api:serverinfo'),
  fetchMetrics:    () => ipcRenderer.invoke('api:metrics'),
  fetchModlist:    () => ipcRenderer.invoke('api:modlist'),
  fetchServers:    () => ipcRenderer.invoke('api:servers'),

  // Discord OAuth
  discordLogin:   () => ipcRenderer.invoke('discord:login'),
  discordLogout:  () => ipcRenderer.invoke('discord:logout'),
  discordGetUser: () => ipcRenderer.invoke('discord:getUser'),

  // Launcher update check
  checkUpdate: () => ipcRenderer.invoke('app:checkUpdate'),

  // Open external URL in default browser (http/https only)
  openExternal: (url) => ipcRenderer.send('open:external', url),

  // SKSE launch
  launchSkse: () => ipcRenderer.invoke('launch:skse'),

  // Game process state — true while Skyrim / the SKSE loader is running
  gameIsRunning: () => ipcRenderer.invoke('game:isRunning'),

  // File install
  startInstall: (mode) => ipcRenderer.send('install:start', mode),
  onInstallProgress: (cb) =>
    ipcRenderer.on('install:progress', (_e, data) => cb(data)),
  onInstallComplete: (cb) =>
    ipcRenderer.on('install:complete',  (_e, data) => cb(data)),
  removeInstallListeners: () => {
    ipcRenderer.removeAllListeners('install:progress')
    ipcRenderer.removeAllListeners('install:complete')
  },

  // Nexus Mods login
  nexusGetUser: ()    => ipcRenderer.invoke('nexus:getUser'),
  nexusLogin:   (key) => ipcRenderer.invoke('nexus:login', key),
  nexusLogout:  ()    => ipcRenderer.invoke('nexus:logout'),
  nexusSsoAvailable: () => ipcRenderer.invoke('nexus:ssoAvailable'),
  nexusSsoLogin:     () => ipcRenderer.invoke('nexus:ssoLogin'),

  // Isolated game copy
  isolatedStatus: () => ipcRenderer.invoke('game:isolatedStatus'),
  createIsolated: () => ipcRenderer.invoke('game:createIsolated'),
  onIsolatedProgress: (cb) => ipcRenderer.on('isolated:progress', (_e, msg) => cb(msg)),
  removeIsolatedListeners: () => ipcRenderer.removeAllListeners('isolated:progress'),

  // MO2 integration
  mo2Status: () => ipcRenderer.invoke('mo2:status'),
  mo2Setup:  () => ipcRenderer.invoke('mo2:setup'),
  mo2Open:   () => ipcRenderer.invoke('mo2:open'),
  onMo2Progress: (cb) => ipcRenderer.on('mo2:progress', (_e, msg) => cb(msg)),
  removeMo2Listeners: () => ipcRenderer.removeAllListeners('mo2:progress'),
})
