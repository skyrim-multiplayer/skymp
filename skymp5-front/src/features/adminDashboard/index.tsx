import React, {
  useCallback,
  useEffect,
  useMemo,
  useRef,
  useState,
} from 'react';
import { useTranslation } from 'react-i18next';

import { FrameButton } from '../../components/FrameButton/FrameButton';
import {
  detectLanguage,
  persistRuntimeLanguage,
} from '../../utils/i18nLanguage';
import { EventsPanel, RevivalEventEntry } from './EventsPanel';
import { PlayerProfileModal } from './PlayerProfileModal';
import { DownedPlayerEntry, RespawnPanel } from './RespawnPanel';
import { ITEM_CATALOG, ITEM_CATEGORIES, ItemCategory } from './itemCatalog';
import './styles.scss';
import {
  filterAdminPlayers,
  formatAdminPos,
  formatAdminTime,
  formatAdminUptime,
  isValidModerationReason,
  isValidDiscordSnowflake,
  isValidWhitelistIdentifier,
  MAX_MODERATION_REASON_LENGTH,
} from './utils';
import { useAdminWebSocket, useAdminWebSocketMessages } from '../../hooks/useAdminWebSocket';

type Tab =
  | 'overview'
  | 'players'
  | 'console'
  | 'logs'
  | 'metrics'
  | 'resources'
  | 'respawn'
  | 'events'
  | 'cfg';
type TopSection =
  | 'players'
  | 'history'
  | 'playerDrops'
  | 'whitelist'
  | 'admins'
  | 'settings'
  | 'system';
type ActiveMenuSurface = 'sidebar' | 'topbar';
type AdminRole = 'admin' | 'moderator' | 'viewer';
type ServerLogLevel = 'info' | 'error';

interface AdminStatus {
  name: string;
  master?: string;
  online: number;
  maxPlayers: number;
  port: number;
  itemPickupMode?: 'allow' | 'minigame' | 'block-all';
  uptimeSec: number;
}

interface AdminUpdateStatus {
  installedVersion: string | null;
  installedCommitSha: string | null;
  installedBuiltAt: string | null;
  latestVersion: string | null;
  updateAvailable: boolean;
  releaseUrl: string;
  publishedAt: string | null;
  changelog: string;
  runtimeReadiness: RuntimeReadinessReport | null;
  error?: string;
}

interface RuntimeReadinessCheck {
  id:
    | 'launcher'
    | 'packageJson'
    | 'entrypoint'
    | 'node'
    | 'npm'
    | 'nodeModules'
    | 'releaseInfo';
  level: 'ok' | 'warn' | 'error';
  code: string;
  message: string;
}

interface RuntimeReadinessReport {
  checkedAt: string;
  overall: 'ok' | 'warn' | 'error';
  checks: RuntimeReadinessCheck[];
}

interface PlayerPos {
  x: number;
  y: number;
  z: number;
}

interface AdminPlayer {
  userId: number;
  actorId: number;
  online?: boolean;
  actorName: string;
  ip: string;
  pos: PlayerPos | number[];
  firstJoinedAt?: number | null;
  lastConnectionAt?: number | null;
  playTimeSec?: number;
}

interface AdminPlayerInventoryEntry {
  baseId?: number;
  count?: number;
  worn?: boolean;
  wornLeft?: boolean;
  [key: string]: unknown;
}

interface AdminPlayerInventorySnapshot {
  ok: boolean;
  userId: number;
  actorId: number;
  profileId: number | null;
  source: 'online' | 'offline-file';
  formDesc?: string;
  updatedAt?: number;
  filePath?: string;
  entryCount: number;
  inventory: {
    entries?: AdminPlayerInventoryEntry[];
    [key: string]: unknown;
  };
}

interface LogEntry {
  ts: number;
  type: 'kick' | 'ban' | 'mute' | 'console' | 'server';
  message: string;
  level?: ServerLogLevel;
}

type AdminHistoryActionType = 'warn' | 'ban' | 'kick' | 'mute';

const extractHistoryUserIdFromLog = (entry: LogEntry): string => {
  const match = entry.message.match(/\buserId=(\d+)\b/i);
  return match?.[1] || '';
};

const inferHistoryActionTypeFromLog = (
  entry: LogEntry,
): 'any' | AdminHistoryActionType => {
  if (entry.type === 'ban' || entry.type === 'kick' || entry.type === 'mute') {
    return entry.type;
  }
  if (entry.type === 'console' && /^warned\s+userId=/i.test(entry.message)) {
    return 'warn';
  }
  return 'any';
};

interface AdminHistoryEntry {
  id: string;
  type: AdminHistoryActionType;
  playerName: string;
  userId: number;
  reason: string;
  author: string;
  ts: number;
}

interface AdminHistoryData {
  entries: AdminHistoryEntry[];
  totalWarns: number;
  newWarns7d: number;
  totalBans: number;
  newBans7d: number;
  admins: string[];
}

interface PlayerDropEntry {
  ts: number;
  userId: number;
  playerName: string;
  type: 'expected' | 'unexpected';
  reason?: string;
}

interface PlayerDropsData {
  expected: PlayerDropEntry[];
  unexpected: PlayerDropEntry[];
  periodStart: number;
  periodEnd: number;
  resourceKicks: Array<{ resource: string; count: number }>;
  environmentChanges: Array<{ ts: number; type: string; description: string }>;
  crashReasons: Array<{ reason: string; count: number }>;
}

const EMPTY_PLAYER_DROPS_DATA: PlayerDropsData = {
  expected: [],
  unexpected: [],
  periodStart: Date.now(),
  periodEnd: Date.now(),
  resourceKicks: [],
  environmentChanges: [],
  crashReasons: [],
};

interface MutedUserEntry {
  userId: number;
  expiresAt: number;
  remainingSec: number;
}

interface BannedUserEntry {
  userId: number;
  permanent: boolean;
  expiresAt: number | null;
  remainingSec: number | null;
}

interface FrontendMetricEntry {
  name: string;
  value: number;
  source: string;
  ts: number;
  receivedAt: number;
  url?: string;
  path?: string;
  clientSource?: string;
  userAgent?: string;
  language?: string;
  platform?: string;
  visibilityState?: string;
  sessionId?: string;
}

interface FrontendMetricsSummary {
  totalCount: number;
  errorCount: number;
  lastReceivedAt: number | null;
  averageValue: number;
  sources: Array<{ name: string; count: number }>;
  names: Array<{ name: string; count: number }>;
}

interface ClientRuntimeEventEntry {
  userId: number;
  ip?: string;
  source: string;
  sessionId?: string;
  event: string;
  level: 'info' | 'warn' | 'error';
  ts: number;
  receivedAt: number;
  details?: string;
}

interface ClientRuntimeEventsSummary {
  totalCount: number;
  errorCount: number;
  warnCount: number;
  lastReceivedAt: number | null;
  sources: Array<{ name: string; count: number }>;
  events: Array<{ name: string; count: number }>;
}

interface AdminCapabilities {
  canKick: boolean;
  canBan: boolean;
  canUnban: boolean;
  canConsole: boolean;
  canViewLogs: boolean;
  canMessage: boolean;
  canMute: boolean;
  canUnmute: boolean;
  canManageRespawn: boolean;
  canWarn: boolean;
}

interface JoinAccessForm {
  mode: 'none' | 'approvedLicense' | 'discordMember' | 'discordRoles';
  rejectionMessage: string;
  approvedLicenses: string;
  approvedDiscordIds: string;
  discordRoleIds: string;
}

interface DiscordBotForm {
  enabled: boolean;
  token: string;
  guildId: string;
  warningsChannelId: string;
}

interface StarterInventoryRow {
  baseId: number;
  count: number;
  worn?: boolean;
  wornLeft?: boolean;
}

interface StartSpawnForm {
  x: number;
  y: number;
  z: number;
  worldOrCell: string;
  angleZ: number;
}

interface SpawnPreset extends StartSpawnForm {
  key: string;
  labelKey: string;
}

interface WorldOrCellOption {
  value: string;
  labelKey: string;
}

interface NpcDefaultSettingsForm {
  spawnInInterior: boolean;
  spawnInExterior: boolean;
  allowHumanoid: boolean;
  allowCreature: boolean;
}

interface CfgFormState {
  serverName: string;
  masterUrl: string;
  masterKey: string;
  port: number;
  maxPlayers: number;
  offlineMode: boolean;
  itemPickupMode: 'allow' | 'minigame' | 'block-all';
  defaultLanguage: string;
  showCharacterNicknames: boolean;
  supervisorEnabled: boolean;
  supervisorStopCommand: string;
  supervisorRestartCommand: string;
  startSpawn: StartSpawnForm;
  npcEnabled: boolean;
  npcDefaultSettings: NpcDefaultSettingsForm;
  joinAccess: JoinAccessForm;
  discordBot: DiscordBotForm;
  starterInventory: StarterInventoryRow[];
}

interface TopbarWhitelistSnapshot {
  mode: JoinAccessForm['mode'];
  rejectionMessage: string;
  approvedLicensesCount: number;
  approvedDiscordIdsCount: number;
  discordRoleIdsCount: number;
  approvedLicenses: string[];
  approvedDiscordIds: string[];
}

interface TopbarAdminSnapshot {
  user: string;
  role: AdminRole;
  capabilities: AdminCapabilities;
}

interface TopbarAdminUserEntry {
  user: string;
  role: AdminRole;
  discordId: string;
  permissionsCount: number;
  permissionsLabel: string;
  auth: {
    password: boolean;
    discord: boolean;
  };
  isCurrentUser: boolean;
  isPrimary: boolean;
}

interface TopbarAdminsData {
  entries: TopbarAdminUserEntry[];
  currentUser: string;
  primaryUser: string;
}

interface TopbarAdminFormState {
  user: string;
  role: AdminRole;
  discordId: string;
}

type CfgEditorTab = 'general' | 'access' | 'inventory' | 'json';

const DEFAULT_CAPABILITIES: AdminCapabilities = {
  canKick: true,
  canBan: true,
  canUnban: true,
  canConsole: true,
  canViewLogs: true,
  canMessage: true,
  canMute: true,
  canUnmute: true,
  canManageRespawn: true,
  canWarn: false,
};

const EMPTY_FRONTEND_METRICS_SUMMARY: FrontendMetricsSummary = {
  totalCount: 0,
  errorCount: 0,
  lastReceivedAt: null,
  averageValue: 0,
  sources: [],
  names: [],
};

const EMPTY_CLIENT_RUNTIME_SUMMARY: ClientRuntimeEventsSummary = {
  totalCount: 0,
  errorCount: 0,
  warnCount: 0,
  lastReceivedAt: null,
  sources: [],
  events: [],
};

const EMPTY_TOPBAR_WHITELIST: TopbarWhitelistSnapshot = {
  mode: 'none',
  rejectionMessage: '',
  approvedLicensesCount: 0,
  approvedDiscordIdsCount: 0,
  discordRoleIdsCount: 0,
  approvedLicenses: [],
  approvedDiscordIds: [],
};

const EMPTY_TOPBAR_ADMIN: TopbarAdminSnapshot = {
  user: '',
  role: 'viewer',
  capabilities: DEFAULT_CAPABILITIES,
};

const EMPTY_TOPBAR_ADMINS_DATA: TopbarAdminsData = {
  entries: [],
  currentUser: '',
  primaryUser: '',
};

const EMPTY_TOPBAR_ADMIN_FORM: TopbarAdminFormState = {
  user: '',
  role: 'moderator',
  discordId: '',
};

const EMPTY_ADMIN_UPDATE_STATUS: AdminUpdateStatus = {
  installedVersion: null,
  installedCommitSha: null,
  installedBuiltAt: null,
  latestVersion: null,
  updateAvailable: false,
  releaseUrl: '',
  publishedAt: null,
  changelog: '',
  runtimeReadiness: null,
};

const REFRESH_INTERVAL_MS = 5000;
const ADMIN_DASHBOARD_STATE_KEY = 'skymp.adminDashboard.state.v1';
const ADMIN_MENU_DEBUG_KEY = 'skymp.adminDashboard.menuDebug';
const ADMIN_WS_RECONNECT_HISTORY_KEY =
  'skymp.adminDashboard.wsReconnectHistory.v1';
const WS_RECONNECT_HISTORY_LIMIT = 10;

type WsReconnectEventKind = 'reconnected' | 'reconnecting' | 'fallback';

interface WsReconnectEvent {
  ts: number;
  kind: WsReconnectEventKind;
}

interface MenuDebugPayload {
  source: string;
  previous: {
    surface: ActiveMenuSurface;
    tab: Tab;
    topSection: TopSection;
  };
  next: {
    surface: ActiveMenuSurface;
    tab: Tab;
    topSection: TopSection;
  };
  visible: boolean;
  ts: number;
}

interface PersistedAdminDashboardState {
  activeMenuSurface?: ActiveMenuSurface;
  activeTab?: Tab;
  activeTopSection?: TopSection;
  playerSearch?: string;
  consoleSearch?: string;
  logTypeFilter?: '' | 'kick' | 'ban' | 'mute' | 'console' | 'server';
  logLevelFilter?: '' | ServerLogLevel;
  logTextFilter?: string;
  logLimit?: number;
  logSinceMinutes?: '' | '15' | '60' | '1440';
  metricLimit?: number;
  metricSourceFilter?: string;
  metricNameFilter?: string;
  eventTypeFilter?: '' | RevivalEventEntry['type'];
  eventLimit?: number;
  consoleHistory?: string[];
  cfgEditorText?: string;
}

const DEFAULT_CFG_FORM: CfgFormState = {
  serverName: '',
  masterUrl: 'https://gateway.skymp.net',
  masterKey: '',
  port: 7777,
  maxPlayers: 100,
  offlineMode: false,
  itemPickupMode: 'allow',
  defaultLanguage: 'en',
  showCharacterNicknames: true,
  supervisorEnabled: false,
  supervisorStopCommand: '',
  supervisorRestartCommand: '',
  startSpawn: {
    x: 133857,
    y: -61130,
    z: 14662,
    worldOrCell: '0x3c',
    angleZ: 72,
  },
  npcEnabled: false,
  npcDefaultSettings: {
    spawnInInterior: true,
    spawnInExterior: true,
    allowHumanoid: true,
    allowCreature: true,
  },
  joinAccess: {
    mode: 'none',
    rejectionMessage:
      'Access denied. Please contact server staff for whitelist approval.',
    approvedLicenses: '',
    approvedDiscordIds: '',
    discordRoleIds: '',
  },
  discordBot: {
    enabled: false,
    token: '',
    guildId: '',
    warningsChannelId: '',
  },
  starterInventory: [],
};

const MASTER_URL_OPTIONS = [
  'https://gateway.skymp.net',
  'https://api.skymp-worlds.net',
] as const;

const normalizeMasterUrl = (value: unknown): string => {
  const normalized = String(value || '')
    .trim()
    .replace(/\/+$/, '')
    .toLowerCase();
  if (
    normalized === 'https://api.skymp-worlds.net' ||
    normalized === 'api.skymp-worlds.net'
  ) {
    return 'https://api.skymp-worlds.net';
  }
  return 'https://gateway.skymp.net';
};

const parseCommaList = (value: string): string[] =>
  value
    .split(',')
    .map((entry) => entry.trim())
    .filter((entry) => entry.length > 0);

const parseItemCodeToBaseId = (code: string): number | null => {
  const normalized = String(code || '')
    .trim()
    .replace(/^0x/i, '')
    .toUpperCase();
  if (!normalized) return null;
  if (!/^[0-9A-F]+$/.test(normalized)) return null;
  const parsed = parseInt(normalized, 16);
  return Number.isFinite(parsed) ? parsed : null;
};

const normalizeWorldOrCell = (value: string): string =>
  String(value || '')
    .trim()
    .toLowerCase();

const SPAWN_PRESETS: SpawnPreset[] = [
  {
    key: 'tamriel',
    labelKey: 'cfgSpawnPreset_tamriel',
    x: 133857,
    y: -61130,
    z: 14662,
    worldOrCell: '0x3c',
    angleZ: 72,
  },
  {
    key: 'whiterun',
    labelKey: 'cfgSpawnPreset_whiterun',
    x: 22659,
    y: -8697,
    z: -3594,
    worldOrCell: '0x1a26f',
    angleZ: 268,
  },
  {
    key: 'riften',
    labelKey: 'cfgSpawnPreset_riften',
    x: 172414.4688,
    y: -99692.1719,
    z: 11136.5918,
    worldOrCell: '0x16bb4',
    angleZ: 177.6169,
  },
  {
    key: 'markarth',
    labelKey: 'cfgSpawnPreset_markarth',
    x: -174156.5781,
    y: 7128.9624,
    z: -3105.9287,
    worldOrCell: '0x16d71',
    angleZ: 166.6154,
  },
  {
    key: 'windhelm',
    labelKey: 'cfgSpawnPreset_windhelm',
    x: 134123.7813,
    y: 36661.9023,
    z: -12252.2842,
    worldOrCell: '0xd45f0',
    angleZ: -83.5598,
  },
];

const WORLD_OR_CELL_HINTS: Record<string, string> = {
  '0x3c': 'cfgWorldOrCellHint_tamriel',
  '0x1a26f': 'cfgWorldOrCellHint_whiterun',
  '0x16bb4': 'cfgWorldOrCellHint_riften',
  '0x16d71': 'cfgWorldOrCellHint_markarth',
  '0xd45f0': 'cfgWorldOrCellHint_windhelm',
};

const WORLD_OR_CELL_OPTIONS: WorldOrCellOption[] = [
  { value: '0x3c', labelKey: 'cfgWorldOrCellOption_tamriel' },
  { value: '0x1a26f', labelKey: 'cfgWorldOrCellOption_whiterun' },
  { value: '0x16bb4', labelKey: 'cfgWorldOrCellOption_riften' },
  { value: '0x16d71', labelKey: 'cfgWorldOrCellOption_markarth' },
  { value: '0xd45f0', labelKey: 'cfgWorldOrCellOption_windhelm' },
];

const matchesSpawnPreset = (
  spawn: StartSpawnForm,
  preset: SpawnPreset,
): boolean => {
  const closeEnough = (left: number, right: number) =>
    Math.abs(left - right) < 0.01;

  return (
    normalizeWorldOrCell(spawn.worldOrCell) ===
      normalizeWorldOrCell(preset.worldOrCell) &&
    closeEnough(spawn.x, preset.x) &&
    closeEnough(spawn.y, preset.y) &&
    closeEnough(spawn.z, preset.z) &&
    closeEnough(spawn.angleZ, preset.angleZ)
  );
};

const toCodeHex = (baseId: number): string =>
  baseId.toString(16).toUpperCase().padStart(8, '0');

const asRole = (value: unknown): AdminRole => {
  if (value === 'admin' || value === 'moderator' || value === 'viewer')
    return value;
  return 'viewer';
};

const AdminDashboard = () => {
  const { t, i18n } = useTranslation();
  const [visible, setVisible] = useState(false);
  const [serverControlAvailable, setServerControlAvailable] = useState(false);
  const [menuDebugEnabled] = useState(() => {
    try {
      const params = new URLSearchParams(window.location.search);
      const debugFromQuery = params.get('adminMenuDebug') === '1';
      const debugFromStorage =
        window.localStorage.getItem(ADMIN_MENU_DEBUG_KEY) === '1';
      return debugFromQuery || debugFromStorage;
    } catch {
      return false;
    }
  });
  const [activeMenuSurface, setActiveMenuSurface] =
    useState<ActiveMenuSurface>('sidebar');
  const [activeTab, setActiveTab] = useState<Tab>('overview');
  const [activeTopSection, setActiveTopSection] =
    useState<TopSection>('settings');
  const [status, setStatus] = useState<AdminStatus | null>(null);
  const [players, setPlayers] = useState<AdminPlayer[]>([]);
  const [bannedUsers, setBannedUsers] = useState<BannedUserEntry[]>([]);
  const [banDurationMinutes, setBanDurationMinutes] = useState(0);
  const [mutedUsers, setMutedUsers] = useState<MutedUserEntry[]>([]);
  const [adminRole, setAdminRole] = useState<AdminRole>('viewer');
  const [adminUser, setAdminUser] = useState('');
  const [capabilities, setCapabilities] =
    useState<AdminCapabilities>(DEFAULT_CAPABILITIES);
  const [playerSearch, setPlayerSearch] = useState('');
  const [statusMsg, setStatusMsg] = useState('');
  const [lastUpdated, setLastUpdated] = useState('');
  const [loading, setLoading] = useState(false);
  const [consoleLines, setConsoleLines] = useState<
    Array<{ text: string; kind: 'input' | 'ok' | 'err' }>
  >([]);
  const [consoleInput, setConsoleInput] = useState('');
  const [consoleHistory, setConsoleHistory] = useState<string[]>([]);
  const [consoleHistoryIndex, setConsoleHistoryIndex] = useState<number | null>(
    null,
  );
  const [consoleSending, setConsoleSending] = useState(false);
  const [consoleSearch, setConsoleSearch] = useState('');
  const [cfgEditorText, setCfgEditorText] = useState('');
  const [cfgEditorStatus, setCfgEditorStatus] = useState('');
  const [cfgEditorLoading, setCfgEditorLoading] = useState(false);
  const [cfgForm, setCfgForm] = useState<CfgFormState>(DEFAULT_CFG_FORM);
  const [inventoryCategory, setInventoryCategory] =
    useState<ItemCategory>('weapons');
  const [inventoryItemCode, setInventoryItemCode] = useState('000139B5');
  const [inventoryCustomCode, setInventoryCustomCode] = useState('');
  const [inventoryCount, setInventoryCount] = useState(1);
  const [inventoryWorn, setInventoryWorn] = useState(false);
  const [inventoryWornLeft, setInventoryWornLeft] = useState(false);
  const [serverConsoleEntries, setServerConsoleEntries] = useState<LogEntry[]>(
    [],
  );
  const [logEntries, setLogEntries] = useState<LogEntry[]>([]);
  const [logTypeFilter, setLogTypeFilter] = useState<
    '' | 'kick' | 'ban' | 'mute' | 'console' | 'server'
  >('');
  const [logLevelFilter, setLogLevelFilter] = useState<'' | ServerLogLevel>('');
  const [logTextFilter, setLogTextFilter] = useState('');
  const [logLimit, setLogLimit] = useState(100);
  const [logSinceMinutes, setLogSinceMinutes] = useState<
    '' | '15' | '60' | '1440'
  >('');
  const [logBeforeTs, setLogBeforeTs] = useState<number | null>(null);
  const [logHasMore, setLogHasMore] = useState(false);
  const [metricEntries, setMetricEntries] = useState<FrontendMetricEntry[]>([]);
  const [metricSummary, setMetricSummary] = useState<FrontendMetricsSummary>(
    EMPTY_FRONTEND_METRICS_SUMMARY,
  );
  const [metricLimit, setMetricLimit] = useState(50);
  const [metricSourceFilter, setMetricSourceFilter] = useState('');
  const [metricNameFilter, setMetricNameFilter] = useState('');
  const [clientRuntimeEntries, setClientRuntimeEntries] = useState<
    ClientRuntimeEventEntry[]
  >([]);
  const [clientRuntimeSummary, setClientRuntimeSummary] =
    useState<ClientRuntimeEventsSummary>(EMPTY_CLIENT_RUNTIME_SUMMARY);
  const [downedPlayers, setDownedPlayers] = useState<DownedPlayerEntry[]>([]);
  const [revivalEvents, setRevivalEvents] = useState<RevivalEventEntry[]>([]);
  const [topbarHistoryData, setTopbarHistoryData] = useState<AdminHistoryData>({
    entries: [],
    totalWarns: 0,
    newWarns7d: 0,
    totalBans: 0,
    newBans7d: 0,
    admins: [],
  });
  const [historySearch, setHistorySearch] = useState('');
  const [historySearchMode, setHistorySearchMode] = useState<
    'actionId' | 'player' | 'reason'
  >('actionId');
  const [historyActionType, setHistoryActionType] = useState<
    'any' | AdminHistoryActionType
  >('any');
  const [historyAdmin, setHistoryAdmin] = useState('any');
  const [topbarPlayerDropsData, setTopbarPlayerDropsData] =
    useState<PlayerDropsData>(EMPTY_PLAYER_DROPS_DATA);
  const [dropsHoursWindow, setDropsHoursWindow] = useState(168);
  const [crashReasonsSortMode, setCrashReasonsSortMode] = useState<
    'count' | 'alphabetical'
  >('count');
  const [crashReasonsLimit, setCrashReasonsLimit] = useState(50);
  const [topbarWhitelistSnapshot, setTopbarWhitelistSnapshot] =
    useState<TopbarWhitelistSnapshot>(EMPTY_TOPBAR_WHITELIST);
  const [topbarAdminSnapshot, setTopbarAdminSnapshot] =
    useState<TopbarAdminSnapshot>(EMPTY_TOPBAR_ADMIN);
  const [topbarAdminsData, setTopbarAdminsData] = useState<TopbarAdminsData>(
    EMPTY_TOPBAR_ADMINS_DATA,
  );
  const [topbarAdminModalOpen, setTopbarAdminModalOpen] = useState(false);
  const [topbarAdminEditingUser, setTopbarAdminEditingUser] = useState<
    string | null
  >(null);
  const [topbarAdminForm, setTopbarAdminForm] = useState<TopbarAdminFormState>(
    EMPTY_TOPBAR_ADMIN_FORM,
  );
  const [topbarAdminSaving, setTopbarAdminSaving] = useState(false);
  const [topbarWhitelistModalOpen, setTopbarWhitelistModalOpen] =
    useState(false);
  const [topbarWhitelistModalIdentifier, setTopbarWhitelistModalIdentifier] =
    useState('');
  const [topbarWhitelistModalSaving, setTopbarWhitelistModalSaving] =
    useState(false);
  const [topbarSystemStatus, setTopbarSystemStatus] =
    useState<AdminStatus | null>(null);
  const [systemDropdownOpen, setSystemDropdownOpen] = useState(false);
  const [systemSectionActive, setSystemSectionActive] = useState(false);
  const [activeSystemSubsection, setActiveSystemSubsection] = useState<
    'masterActions' | 'diagnostics' | 'consoleLog' | 'actionLog'
  >('masterActions');
  const [topbarWhitelistSearch, setTopbarWhitelistSearch] = useState('');

  const [topbarPlayersSearch, setTopbarPlayersSearch] = useState('');
  const [cfgEditorTab, setCfgEditorTab] = useState<CfgEditorTab>('general');
  const [cfgEditorSaving, setCfgEditorSaving] = useState(false);
  const [updateStatus, setUpdateStatus] = useState<AdminUpdateStatus>(
    EMPTY_ADMIN_UPDATE_STATUS,
  );
  const [updateStatusLoading, setUpdateStatusLoading] = useState(false);
  const [updateModalOpen, setUpdateModalOpen] = useState(false);
  const [topbarPlayersSearchMode, setTopbarPlayersSearchMode] = useState<
    'name' | 'id'
  >('name');
  const [topbarPlayersFilter, setTopbarPlayersFilter] = useState<
    'none' | 'online' | 'offline'
  >('none');
  const [eventTypeFilter, setEventTypeFilter] = useState<
    '' | RevivalEventEntry['type']
  >('');
  const [eventLimit, setEventLimit] = useState(100);
  const [sendMsgTargetId, setSendMsgTargetId] = useState<number | null>(null);
  const [sendMsgTargetName, setSendMsgTargetName] = useState('');
  const [sendMsgText, setSendMsgText] = useState('');
  const [sendMsgSending, setSendMsgSending] = useState(false);
  const [profileModalUserId, setProfileModalUserId] = useState<number | null>(null);
  const [profileModalName, setProfileModalName] = useState('');
  const [warnDialogOpen, setWarnDialogOpen] = useState(false);
  const [warnTargetId, setWarnTargetId] = useState<number | null>(null);
  const [warnTargetName, setWarnTargetName] = useState('');
  const [warnMessage, setWarnMessage] = useState('');
  const [warnReason, setWarnReason] = useState('');
  const [warnSending, setWarnSending] = useState(false);
  const [inventoryTargetUserId, setInventoryTargetUserId] = useState<
    number | null
  >(null);
  const [inventoryTargetName, setInventoryTargetName] = useState('');
  const [inventoryLoading, setInventoryLoading] = useState(false);
  const [inventoryError, setInventoryError] = useState('');
  const [playerInventory, setPlayerInventory] =
    useState<AdminPlayerInventorySnapshot | null>(null);
  const [sidebarActionSending, setSidebarActionSending] = useState<
    'kick-all' | 'announcement' | 'server-stop' | 'server-restart' | null
  >(null);
  const [muteDurationMinutes, setMuteDurationMinutes] = useState(10);
  const [moderationReason, setModerationReason] = useState('');
  const moderationReasonInvalid = !isValidModerationReason(moderationReason);
  const warnReasonInvalid = !isValidModerationReason(warnReason);
  const [nowTs, setNowTs] = useState(() => Date.now());
  const intervalRef = useRef<ReturnType<typeof setInterval> | null>(null);
  const consoleEndRef = useRef<HTMLDivElement>(null);
  const menuDebugPrevRef = useRef<{
    surface: ActiveMenuSurface;
    tab: Tab;
    topSection: TopSection;
  } | null>(null);

  const emitMenuDebug = useCallback(
    (payload: MenuDebugPayload) => {
      if (!menuDebugEnabled) return;

      const stamp = new Date(payload.ts).toISOString();
      console.info(
        `[AdminMenuDebug ${stamp}] ${payload.source} | ${payload.previous.surface}/${payload.previous.tab}/${payload.previous.topSection} -> ${payload.next.surface}/${payload.next.tab}/${payload.next.topSection} | visible=${payload.visible}`,
      );
      window.dispatchEvent(
        new CustomEvent('adminMenuDebug', { detail: payload }),
      );

      void fetch('/api/admin/menu-debug', {
        method: 'POST',
        credentials: 'same-origin',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload),
      }).catch(() => {
        // keep UI stable even when debug transport fails
      });
    },
    [menuDebugEnabled],
  );

  useEffect(() => {
    if (!menuDebugEnabled) return;
    try {
      window.localStorage.setItem(ADMIN_MENU_DEBUG_KEY, '1');
    } catch {
      // ignore storage write failures
    }

    const startupPayload: MenuDebugPayload = {
      source: 'debug-enabled',
      previous: {
        surface: activeMenuSurface,
        tab: activeTab,
        topSection: activeTopSection,
      },
      next: {
        surface: activeMenuSurface,
        tab: activeTab,
        topSection: activeTopSection,
      },
      visible,
      ts: Date.now(),
    };
    emitMenuDebug(startupPayload);

    void fetch('/api/admin/menu-debug', {
      method: 'GET',
      credentials: 'same-origin',
    })
      .then(async (response) => {
        if (!response.ok) return;
        const info = await response.json().catch(() => null);
        if (info?.path) {
          console.info(
            `[AdminMenuDebug] log file path: ${info.path} (exists=${Boolean(
              info.exists,
            )}, size=${Number(info.size) || 0})`,
          );
        }
      })
      .catch(() => {
        // ignore debug status read failures
      });
  }, [emitMenuDebug, menuDebugEnabled]);

  useEffect(() => {
    try {
      const raw = window.localStorage.getItem(ADMIN_DASHBOARD_STATE_KEY);
      if (!raw) return;
      const persisted = JSON.parse(raw) as PersistedAdminDashboardState;

      if (
        persisted.activeMenuSurface === 'sidebar' ||
        persisted.activeMenuSurface === 'topbar'
      ) {
        setActiveMenuSurface(persisted.activeMenuSurface);
      }

      if (
        persisted.activeTab &&
        [
          'overview',
          'players',
          'console',
          'logs',
          'metrics',
          'respawn',
          'events',
          'cfg',
        ].includes(persisted.activeTab)
      ) {
        setActiveTab(persisted.activeTab);
      }
      if (
        persisted.activeTopSection &&
        [
          'players',
          'history',
          'playerDrops',
          'whitelist',
          'admins',
          'settings',
          'system',
        ].includes(persisted.activeTopSection)
      ) {
        setActiveTopSection(persisted.activeTopSection);
      }
      if (typeof persisted.playerSearch === 'string')
        setPlayerSearch(persisted.playerSearch);
      if (typeof persisted.consoleSearch === 'string')
        setConsoleSearch(persisted.consoleSearch);
      if (
        persisted.logTypeFilter === '' ||
        persisted.logTypeFilter === 'kick' ||
        persisted.logTypeFilter === 'ban' ||
        persisted.logTypeFilter === 'mute' ||
        persisted.logTypeFilter === 'console' ||
        persisted.logTypeFilter === 'server'
      ) {
        setLogTypeFilter(persisted.logTypeFilter);
      }
      if (
        persisted.logLevelFilter === '' ||
        persisted.logLevelFilter === 'info' ||
        persisted.logLevelFilter === 'error'
      ) {
        setLogLevelFilter(persisted.logLevelFilter);
      }
      if (typeof persisted.logTextFilter === 'string')
        setLogTextFilter(persisted.logTextFilter);
      if (persisted.logLimit && [25, 50, 100, 200].includes(persisted.logLimit))
        setLogLimit(persisted.logLimit);
      if (
        persisted.logSinceMinutes === '' ||
        persisted.logSinceMinutes === '15' ||
        persisted.logSinceMinutes === '60' ||
        persisted.logSinceMinutes === '1440'
      ) {
        setLogSinceMinutes(persisted.logSinceMinutes);
      }
      if (
        persisted.metricLimit &&
        [25, 50, 100, 200].includes(persisted.metricLimit)
      )
        setMetricLimit(persisted.metricLimit);
      if (typeof persisted.metricSourceFilter === 'string')
        setMetricSourceFilter(persisted.metricSourceFilter);
      if (typeof persisted.metricNameFilter === 'string')
        setMetricNameFilter(persisted.metricNameFilter);
      if (
        persisted.eventTypeFilter === '' ||
        persisted.eventTypeFilter === 'downed' ||
        persisted.eventTypeFilter === 'revived' ||
        persisted.eventTypeFilter === 'respawn_disabled' ||
        persisted.eventTypeFilter === 'respawn_enabled' ||
        persisted.eventTypeFilter === 'auto_revived'
      ) {
        setEventTypeFilter(persisted.eventTypeFilter);
      }
      if (
        persisted.eventLimit &&
        [25, 50, 100, 200].includes(persisted.eventLimit)
      )
        setEventLimit(persisted.eventLimit);
      if (Array.isArray(persisted.consoleHistory)) {
        setConsoleHistory(
          persisted.consoleHistory
            .filter((entry) => typeof entry === 'string')
            .slice(0, 30),
        );
      }
      if (typeof persisted.cfgEditorText === 'string') {
        setCfgEditorText(persisted.cfgEditorText);
      }
    } catch {
      // ignore persisted state parsing issues
    }
  }, []);

  useEffect(() => {
    try {
      const stateToPersist: PersistedAdminDashboardState = {
        activeMenuSurface,
        activeTab,
        activeTopSection,
        playerSearch,
        consoleSearch,
        logTypeFilter,
        logLevelFilter,
        logTextFilter,
        logLimit,
        logSinceMinutes,
        metricLimit,
        metricSourceFilter,
        metricNameFilter,
        eventTypeFilter,
        eventLimit,
        consoleHistory,
        cfgEditorText,
      };
      window.localStorage.setItem(
        ADMIN_DASHBOARD_STATE_KEY,
        JSON.stringify(stateToPersist),
      );
    } catch {
      // ignore storage write failures
    }
  }, [
    activeMenuSurface,
    activeTab,
    activeTopSection,
    cfgEditorText,
    consoleHistory,
    consoleSearch,
    eventLimit,
    eventTypeFilter,
    logLevelFilter,
    logLimit,
    logSinceMinutes,
    logTextFilter,
    logTypeFilter,
    metricLimit,
    metricNameFilter,
    metricSourceFilter,
    playerSearch,
  ]);

  useEffect(() => {
    const current = {
      surface: activeMenuSurface,
      tab: activeTab,
      topSection: activeTopSection,
    };
    const previous = menuDebugPrevRef.current;

    if (!previous) {
      menuDebugPrevRef.current = current;
      return;
    }

    if (
      previous.surface !== current.surface ||
      previous.tab !== current.tab ||
      previous.topSection !== current.topSection
    ) {
      emitMenuDebug({
        source: 'state-change',
        previous,
        next: current,
        visible,
        ts: Date.now(),
      });
      menuDebugPrevRef.current = current;
    }
  }, [activeMenuSurface, activeTab, activeTopSection, emitMenuDebug, visible]);

  const setForbiddenAwareStatus = useCallback(
    (res: Response, successText: string) => {
      if (res.ok) {
        setStatusMsg(successText);
        return;
      }

      if (res.status === 403) {
        setStatusMsg(t('adminDashboard.noPermission'));
        return;
      }

      if (res.status === 429) {
        setStatusMsg(t('adminDashboard.rateLimited'));
        return;
      }

      setStatusMsg(t('adminDashboard.apiError'));
    },
    [t],
  );

  const fetchData = useCallback(async () => {
    try {
      const [statusRes, playersRes] = await Promise.all([
        fetch('/api/admin/status'),
        fetch('/api/admin/players'),
      ]);
      if (!statusRes.ok || !playersRes.ok) {
        if (statusRes.status === 429 || playersRes.status === 429) {
          setStatusMsg(t('adminDashboard.rateLimited'));
        } else {
          setStatusMsg(t('adminDashboard.apiError'));
        }
        return;
      }

      const statusData: AdminStatus = await statusRes.json();
      const playersData: AdminPlayer[] = await playersRes.json();
      setStatus(statusData);
      setPlayers(playersData);
      setLastUpdated(new Date().toLocaleTimeString());
      setStatusMsg('');
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  }, [t]);

  const fetchCapabilities = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/capabilities');
      if (!res.ok) return;

      const payload = await res.json();
      setAdminRole(asRole(payload?.role));
      setAdminUser(typeof payload?.user === 'string' ? payload.user : '');
      setServerControlAvailable(Boolean(payload?.serverControlAvailable));
      setCapabilities({
        canKick:
          typeof payload?.canKick === 'boolean'
            ? payload.canKick
            : DEFAULT_CAPABILITIES.canKick,
        canBan:
          typeof payload?.canBan === 'boolean'
            ? payload.canBan
            : DEFAULT_CAPABILITIES.canBan,
        canUnban:
          typeof payload?.canUnban === 'boolean'
            ? payload.canUnban
            : DEFAULT_CAPABILITIES.canUnban,
        canConsole:
          typeof payload?.canConsole === 'boolean'
            ? payload.canConsole
            : DEFAULT_CAPABILITIES.canConsole,
        canViewLogs:
          typeof payload?.canViewLogs === 'boolean'
            ? payload.canViewLogs
            : DEFAULT_CAPABILITIES.canViewLogs,
        canMessage:
          typeof payload?.canMessage === 'boolean'
            ? payload.canMessage
            : DEFAULT_CAPABILITIES.canMessage,
        canMute:
          typeof payload?.canMute === 'boolean'
            ? payload.canMute
            : DEFAULT_CAPABILITIES.canMute,
        canUnmute:
          typeof payload?.canUnmute === 'boolean'
            ? payload.canUnmute
            : DEFAULT_CAPABILITIES.canUnmute,
        canManageRespawn:
          typeof payload?.canManageRespawn === 'boolean'
            ? payload.canManageRespawn
            : DEFAULT_CAPABILITIES.canManageRespawn,
        canWarn:
          typeof payload?.canWarn === 'boolean'
            ? payload.canWarn
            : DEFAULT_CAPABILITIES.canWarn,
      });
    } catch {
      // silently ignore
    }
  }, []);

  const fetchUpdateStatus = useCallback(async () => {
    setUpdateStatusLoading(true);
    try {
      const res = await fetch('/api/admin/update-status');
      if (!res.ok) return;
      const payload = (await res.json()) as Partial<AdminUpdateStatus>;

      const runtimeReadinessRaw =
        payload.runtimeReadiness &&
        typeof payload.runtimeReadiness === 'object' &&
        Array.isArray((payload.runtimeReadiness as any).checks)
          ? (payload.runtimeReadiness as RuntimeReadinessReport)
          : null;

      setUpdateStatus({
        installedVersion:
          typeof payload.installedVersion === 'string'
            ? payload.installedVersion
            : null,
        installedCommitSha:
          typeof payload.installedCommitSha === 'string'
            ? payload.installedCommitSha
            : null,
        installedBuiltAt:
          typeof payload.installedBuiltAt === 'string'
            ? payload.installedBuiltAt
            : null,
        latestVersion:
          typeof payload.latestVersion === 'string'
            ? payload.latestVersion
            : null,
        updateAvailable: Boolean(payload.updateAvailable),
        releaseUrl: typeof payload.releaseUrl === 'string' ? payload.releaseUrl : '',
        publishedAt:
          typeof payload.publishedAt === 'string' ? payload.publishedAt : null,
        changelog: typeof payload.changelog === 'string' ? payload.changelog : '',
        runtimeReadiness: runtimeReadinessRaw,
        error: typeof payload.error === 'string' ? payload.error : undefined,
      });
    } catch {
      // silently ignore update status failures to keep dashboard usable offline
    } finally {
      setUpdateStatusLoading(false);
    }
  }, []);

  const fetchBans = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/bans');
      if (!res.ok) return;
      const bans = await res.json();
      setBannedUsers(Array.isArray(bans) ? (bans as BannedUserEntry[]) : []);
    } catch {
      // silently ignore
    }
  }, []);

  const fetchMutes = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/mutes');
      if (!res.ok) return;
      const mutes = await res.json();
      const parsed = Array.isArray(mutes) ? (mutes as MutedUserEntry[]) : [];
      setMutedUsers(parsed);
    } catch {
      // silently ignore
    }
  }, []);

  const fetchLogs = useCallback(async () => {
    try {
      if (!capabilities.canViewLogs) {
        setLogEntries([]);
        setLogHasMore(false);
        return;
      }

      const params = new URLSearchParams();
      if (logTypeFilter) params.set('type', logTypeFilter);
      if (logLevelFilter) params.set('level', logLevelFilter);
      params.set('limit', String(logLimit));
      if (logSinceMinutes) params.set('sinceMinutes', logSinceMinutes);
      if (logBeforeTs !== null) params.set('beforeTs', String(logBeforeTs));

      const res = await fetch(`/api/admin/logs?${params.toString()}`);
      if (!res.ok) {
        if (res.status === 403) {
          setLogEntries([]);
          setLogHasMore(false);
        }
        return;
      }

      const entries = await res.json();
      const parsed = Array.isArray(entries) ? (entries as LogEntry[]) : [];
      setLogEntries(parsed);
      setLogHasMore(parsed.length === logLimit);
    } catch {
      // silently ignore
    }
  }, [
    capabilities.canViewLogs,
    logBeforeTs,
    logLevelFilter,
    logLimit,
    logSinceMinutes,
    logTypeFilter,
  ]);

  // --- WebSocket for Live Console ---
  useEffect(() => {
    if (
      !visible ||
      activeMenuSurface !== 'sidebar' ||
      activeTab !== 'console' ||
      !capabilities.canViewLogs
    )
      return;
    let ws: WebSocket | null = null;
    let closed = false;
    let reconnectTimeout: any = null;

    function connect() {
      ws = new window.WebSocket(
        `${window.location.protocol === 'https:' ? 'wss' : 'ws'}://${
          window.location.host
        }/api/admin/live-console`,
      );
      ws.onopen = () => {
        // Optionally: set status
      };
      ws.onmessage = (event) => {
        try {
          const entry = JSON.parse(event.data);
          setServerConsoleEntries((prev) => {
            // Avoid duplicates if initial batch overlaps
            if (
              prev.length > 0 &&
              prev[prev.length - 1].ts === entry.ts &&
              prev[prev.length - 1].message === entry.message
            )
              return prev;
            return [...prev, entry].slice(-200);
          });
        } catch {}
      };
      ws.onclose = () => {
        if (!closed) reconnectTimeout = setTimeout(connect, 2000);
      };
      ws.onerror = () => {
        ws?.close();
      };
    }
    connect();
    return () => {
      closed = true;
      if (ws) ws.close();
      if (reconnectTimeout) clearTimeout(reconnectTimeout);
    };
  }, [visible, activeMenuSurface, activeTab, capabilities.canViewLogs]);

  const fetchFrontendMetrics = useCallback(async () => {
    try {
      if (!capabilities.canViewLogs) {
        setMetricEntries([]);
        setMetricSummary(EMPTY_FRONTEND_METRICS_SUMMARY);
        return;
      }

      const params = new URLSearchParams();
      params.set('limit', String(metricLimit));
      if (metricSourceFilter) params.set('source', metricSourceFilter);
      if (metricNameFilter.trim()) params.set('name', metricNameFilter.trim());

      const res = await fetch(
        `/api/admin/frontend-metrics?${params.toString()}`,
      );
      if (!res.ok) {
        if (res.status === 403) {
          setMetricEntries([]);
          setMetricSummary(EMPTY_FRONTEND_METRICS_SUMMARY);
        }
        return;
      }

      const payload = await res.json();
      setMetricEntries(Array.isArray(payload?.entries) ? payload.entries : []);
      setMetricSummary(payload?.summary || EMPTY_FRONTEND_METRICS_SUMMARY);
    } catch {
      // silently ignore
    }
  }, [
    capabilities.canViewLogs,
    metricLimit,
    metricNameFilter,
    metricSourceFilter,
  ]);

  const fetchClientRuntimeEvents = useCallback(async () => {
    try {
      if (!capabilities.canViewLogs) {
        setClientRuntimeEntries([]);
        setClientRuntimeSummary(EMPTY_CLIENT_RUNTIME_SUMMARY);
        return;
      }

      const params = new URLSearchParams();
      params.set('limit', String(metricLimit));

      const res = await fetch(
        `/api/admin/client-runtime-events?${params.toString()}`,
      );
      if (!res.ok) {
        if (res.status === 403) {
          setClientRuntimeEntries([]);
          setClientRuntimeSummary(EMPTY_CLIENT_RUNTIME_SUMMARY);
        }
        return;
      }

      const payload = await res.json();
      setClientRuntimeEntries(
        Array.isArray(payload?.entries) ? payload.entries : [],
      );
      setClientRuntimeSummary(payload?.summary || EMPTY_CLIENT_RUNTIME_SUMMARY);
    } catch {
      // silently ignore
    }
  }, [capabilities.canViewLogs, metricLimit]);

  const fetchDownedPlayers = useCallback(async () => {
    try {
      if (!capabilities.canManageRespawn) {
        setDownedPlayers([]);
        return;
      }

      const res = await fetch('/api/admin/respawn-status');
      if (!res.ok) {
        if (res.status === 403) {
          setDownedPlayers([]);
        }
        return;
      }

      const payload = await res.json();
      setDownedPlayers(
        Array.isArray(payload) ? (payload as DownedPlayerEntry[]) : [],
      );
    } catch {
      // silently ignore
    }
  }, [capabilities.canManageRespawn]);

  const fetchRevivalEvents = useCallback(async () => {
    try {
      if (!capabilities.canViewLogs) {
        setRevivalEvents([]);
        return;
      }

      const params = new URLSearchParams();
      if (eventTypeFilter) params.set('type', eventTypeFilter);
      params.set('limit', String(eventLimit));

      const res = await fetch(`/api/admin/events?${params.toString()}`);
      if (!res.ok) {
        if (res.status === 403) {
          setRevivalEvents([]);
        }
        return;
      }

      const payload = await res.json();
      setRevivalEvents(
        Array.isArray(payload) ? (payload as RevivalEventEntry[]) : [],
      );
    } catch {
      // silently ignore
    }
  }, [capabilities.canViewLogs, eventLimit, eventTypeFilter]);

  const mapJsonToCfgForm = useCallback((jsonText: string): CfgFormState => {
    const parsed = JSON.parse(jsonText) as Record<string, unknown>;
    const joinAccessRaw =
      parsed.joinAccess && typeof parsed.joinAccess === 'object'
        ? (parsed.joinAccess as Record<string, unknown>)
        : {};
    const discordBotRaw =
      parsed.discordBot && typeof parsed.discordBot === 'object'
        ? (parsed.discordBot as Record<string, unknown>)
        : {};
    const startSpawnRaw =
      parsed.startSpawn && typeof parsed.startSpawn === 'object'
        ? (parsed.startSpawn as Record<string, unknown>)
        : {};
    const startSpawnPos = Array.isArray(startSpawnRaw.pos)
      ? (startSpawnRaw.pos as unknown[])
      : [];
    const npcSettingsRaw =
      parsed.npcSettings && typeof parsed.npcSettings === 'object'
        ? (parsed.npcSettings as Record<string, unknown>)
        : {};
    const npcDefaultSettingsRaw =
      npcSettingsRaw.default && typeof npcSettingsRaw.default === 'object'
        ? (npcSettingsRaw.default as Record<string, unknown>)
        : {};
    const starterInventoryRaw =
      parsed.starterInventory && typeof parsed.starterInventory === 'object'
        ? (parsed.starterInventory as Record<string, unknown>)
        : {};

    const entriesRaw = Array.isArray(starterInventoryRaw.entries)
      ? starterInventoryRaw.entries
      : [];
    const starterInventory = entriesRaw
      .map((entry) => {
        if (!entry || typeof entry !== 'object') return null;
        const typed = entry as Record<string, unknown>;
        const baseId = Number(typed.baseId);
        const count = Number(typed.count);
        if (!Number.isFinite(baseId) || !Number.isFinite(count)) return null;
        return {
          baseId: Math.max(0, Math.floor(baseId)),
          count: Math.max(1, Math.floor(count)),
          worn: Boolean(typed.worn),
          wornLeft: Boolean(typed.wornLeft),
        } as StarterInventoryRow;
      })
      .filter((row): row is StarterInventoryRow => row !== null);

    return {
      serverName: String(parsed.name || ''),
      masterUrl: normalizeMasterUrl(parsed.master),
      masterKey: String(parsed.masterKey || ''),
      port: Number.isFinite(Number(parsed.port))
        ? Math.max(1, Math.floor(Number(parsed.port)))
        : 7777,
      maxPlayers: Number.isFinite(Number(parsed.maxPlayers))
        ? Math.max(1, Math.floor(Number(parsed.maxPlayers)))
        : 100,
      offlineMode: Boolean(parsed.offlineMode),
      itemPickupMode: (['allow', 'minigame', 'block-all'].includes(
        String(parsed.itemPickupMode),
      )
        ? String(parsed.itemPickupMode)
        : DEFAULT_CFG_FORM.itemPickupMode) as CfgFormState['itemPickupMode'],
      defaultLanguage: String(
        (parsed.localeRouting as Record<string, unknown> | undefined)
          ?.defaultLanguage || 'en',
      ),
      showCharacterNicknames:
        parsed.showCharacterNicknames === undefined
          ? DEFAULT_CFG_FORM.showCharacterNicknames
          : Boolean(parsed.showCharacterNicknames),
      startSpawn: {
        x: Number.isFinite(Number(startSpawnPos[0]))
          ? Number(startSpawnPos[0])
          : DEFAULT_CFG_FORM.startSpawn.x,
        y: Number.isFinite(Number(startSpawnPos[1]))
          ? Number(startSpawnPos[1])
          : DEFAULT_CFG_FORM.startSpawn.y,
        z: Number.isFinite(Number(startSpawnPos[2]))
          ? Number(startSpawnPos[2])
          : DEFAULT_CFG_FORM.startSpawn.z,
        worldOrCell: String(
          startSpawnRaw.worldOrCell || DEFAULT_CFG_FORM.startSpawn.worldOrCell,
        ),
        angleZ: Number.isFinite(Number(startSpawnRaw.angleZ))
          ? Number(startSpawnRaw.angleZ)
          : DEFAULT_CFG_FORM.startSpawn.angleZ,
      },
      npcEnabled: Boolean(parsed.npcEnabled),
      npcDefaultSettings: {
        spawnInInterior:
          npcDefaultSettingsRaw.spawnInInterior === undefined
            ? DEFAULT_CFG_FORM.npcDefaultSettings.spawnInInterior
            : Boolean(npcDefaultSettingsRaw.spawnInInterior),
        spawnInExterior:
          npcDefaultSettingsRaw.spawnInExterior === undefined
            ? DEFAULT_CFG_FORM.npcDefaultSettings.spawnInExterior
            : Boolean(npcDefaultSettingsRaw.spawnInExterior),
        allowHumanoid:
          npcDefaultSettingsRaw.allowHumanoid === undefined
            ? DEFAULT_CFG_FORM.npcDefaultSettings.allowHumanoid
            : Boolean(npcDefaultSettingsRaw.allowHumanoid),
        allowCreature:
          npcDefaultSettingsRaw.allowCreature === undefined
            ? DEFAULT_CFG_FORM.npcDefaultSettings.allowCreature
            : Boolean(npcDefaultSettingsRaw.allowCreature),
      },
      joinAccess: {
        mode: ([
          'none',
          'approvedLicense',
          'discordMember',
          'discordRoles',
        ].includes(String(joinAccessRaw.mode))
          ? String(joinAccessRaw.mode)
          : 'none') as JoinAccessForm['mode'],
        rejectionMessage: String(
          joinAccessRaw.rejectionMessage ||
            DEFAULT_CFG_FORM.joinAccess.rejectionMessage,
        ),
        approvedLicenses: Array.isArray(joinAccessRaw.approvedLicenses)
          ? joinAccessRaw.approvedLicenses.map((v) => String(v)).join(', ')
          : '',
        approvedDiscordIds: Array.isArray(joinAccessRaw.approvedDiscordIds)
          ? joinAccessRaw.approvedDiscordIds.map((v) => String(v)).join(', ')
          : '',
        discordRoleIds: Array.isArray(joinAccessRaw.discordRoleIds)
          ? joinAccessRaw.discordRoleIds.map((v) => String(v)).join(', ')
          : '',
      },
      discordBot: {
        enabled: Boolean(discordBotRaw.enabled),
        token: String(discordBotRaw.token || ''),
        guildId: String(discordBotRaw.guildId || ''),
        warningsChannelId: String(discordBotRaw.warningsChannelId || ''),
      },
      supervisorEnabled: Boolean(
        (parsed.supervisor as Record<string, unknown> | undefined)?.enabled,
      ),
      supervisorStopCommand: String(
        (parsed.supervisor as Record<string, unknown> | undefined)?.stopCommand || '',
      ).trim(),
      supervisorRestartCommand: String(
        (parsed.supervisor as Record<string, unknown> | undefined)?.restartCommand || '',
      ).trim(),
      starterInventory,
    };
  }, []);

  const fetchTopbarHistory = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/history?limit=200');
      if (!res.ok) {
        setTopbarHistoryData({
          entries: [],
          totalWarns: 0,
          newWarns7d: 0,
          totalBans: 0,
          newBans7d: 0,
          admins: [],
        });
        return;
      }

      const payload = (await res.json()) as Partial<AdminHistoryData>;
      setTopbarHistoryData({
        entries: Array.isArray(payload.entries) ? payload.entries : [],
        totalWarns: Number(payload.totalWarns ?? 0),
        newWarns7d: Number(payload.newWarns7d ?? 0),
        totalBans: Number(payload.totalBans ?? 0),
        newBans7d: Number(payload.newBans7d ?? 0),
        admins: Array.isArray(payload.admins) ? payload.admins : [],
      });
    } catch {
      setTopbarHistoryData({
        entries: [],
        totalWarns: 0,
        newWarns7d: 0,
        totalBans: 0,
        newBans7d: 0,
        admins: [],
      });
    }
  }, []);

  const fetchTopbarPlayerDrops = useCallback(
    async (
      hours = 168,
      crashLimit = 50,
      sort: 'count' | 'alphabetical' = 'count',
    ) => {
      try {
        const res = await fetch(
          `/api/admin/player-drops?hours=${hours}&crashLimit=${crashLimit}&sort=${sort}`,
        );
        if (!res.ok) {
          setTopbarPlayerDropsData(EMPTY_PLAYER_DROPS_DATA);
          return;
        }
        const payload = (await res.json()) as Partial<PlayerDropsData>;
        setTopbarPlayerDropsData({
          expected: Array.isArray(payload.expected) ? payload.expected : [],
          unexpected: Array.isArray(payload.unexpected)
            ? payload.unexpected
            : [],
          periodStart: Number(payload.periodStart ?? Date.now()),
          periodEnd: Number(payload.periodEnd ?? Date.now()),
          resourceKicks: Array.isArray(payload.resourceKicks)
            ? payload.resourceKicks
            : [],
          environmentChanges: Array.isArray(payload.environmentChanges)
            ? payload.environmentChanges
            : [],
          crashReasons: Array.isArray(payload.crashReasons)
            ? payload.crashReasons
            : [],
        });
      } catch {
        setTopbarPlayerDropsData(EMPTY_PLAYER_DROPS_DATA);
      }
    },
    [],
  );

  const fetchTopbarCfgSnapshots = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/cfg/server-settings');
      if (!res.ok) {
        setTopbarWhitelistSnapshot(EMPTY_TOPBAR_WHITELIST);
        return;
      }

      const payload = await res.json();
      const jsonText =
        typeof payload?.json === 'string'
          ? payload.json
          : JSON.stringify(payload ?? {}, null, 2);
      const cfg = mapJsonToCfgForm(jsonText);

      setTopbarWhitelistSnapshot({
        mode: cfg.joinAccess.mode,
        rejectionMessage: cfg.joinAccess.rejectionMessage,
        approvedLicensesCount: parseCommaList(cfg.joinAccess.approvedLicenses)
          .length,
        approvedDiscordIdsCount: parseCommaList(
          cfg.joinAccess.approvedDiscordIds,
        ).length,
        discordRoleIdsCount: parseCommaList(cfg.joinAccess.discordRoleIds)
          .length,
        approvedLicenses: parseCommaList(cfg.joinAccess.approvedLicenses),
        approvedDiscordIds: parseCommaList(cfg.joinAccess.approvedDiscordIds),
      });
    } catch {
      setTopbarWhitelistSnapshot(EMPTY_TOPBAR_WHITELIST);
    }
  }, [mapJsonToCfgForm]);

  const fetchTopbarAdminSnapshot = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/capabilities');
      if (!res.ok) {
        setTopbarAdminSnapshot(EMPTY_TOPBAR_ADMIN);
        return;
      }

      const payload = await res.json();
      setTopbarAdminSnapshot({
        user: typeof payload?.user === 'string' ? payload.user : '',
        role: asRole(payload?.role),
        capabilities: {
          canKick:
            typeof payload?.canKick === 'boolean'
              ? payload.canKick
              : DEFAULT_CAPABILITIES.canKick,
          canBan:
            typeof payload?.canBan === 'boolean'
              ? payload.canBan
              : DEFAULT_CAPABILITIES.canBan,
          canUnban:
            typeof payload?.canUnban === 'boolean'
              ? payload.canUnban
              : DEFAULT_CAPABILITIES.canUnban,
          canConsole:
            typeof payload?.canConsole === 'boolean'
              ? payload.canConsole
              : DEFAULT_CAPABILITIES.canConsole,
          canViewLogs:
            typeof payload?.canViewLogs === 'boolean'
              ? payload.canViewLogs
              : DEFAULT_CAPABILITIES.canViewLogs,
          canMessage:
            typeof payload?.canMessage === 'boolean'
              ? payload.canMessage
              : DEFAULT_CAPABILITIES.canMessage,
          canMute:
            typeof payload?.canMute === 'boolean'
              ? payload.canMute
              : DEFAULT_CAPABILITIES.canMute,
          canUnmute:
            typeof payload?.canUnmute === 'boolean'
              ? payload.canUnmute
              : DEFAULT_CAPABILITIES.canUnmute,
          canManageRespawn:
            typeof payload?.canManageRespawn === 'boolean'
              ? payload.canManageRespawn
              : DEFAULT_CAPABILITIES.canManageRespawn,
          canWarn:
            typeof payload?.canWarn === 'boolean'
              ? payload.canWarn
              : DEFAULT_CAPABILITIES.canWarn,
        },
      });
    } catch {
      setTopbarAdminSnapshot(EMPTY_TOPBAR_ADMIN);
    }
  }, []);

  const fetchTopbarAdmins = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/admin-users');
      if (!res.ok) {
        setTopbarAdminsData(EMPTY_TOPBAR_ADMINS_DATA);
        return;
      }
      const payload = await res.json();
      const entries = Array.isArray(payload?.entries) ? payload.entries : [];
      setTopbarAdminsData({
        entries: entries.map((entry: any) => ({
          user: String(entry?.user || ''),
          role: asRole(entry?.role),
          discordId: String(entry?.discordId || ''),
          permissionsCount: Number(entry?.permissionsCount || 0),
          permissionsLabel: String(entry?.permissionsLabel || ''),
          auth: {
            password: Boolean(entry?.auth?.password),
            discord: Boolean(entry?.auth?.discord),
          },
          isCurrentUser: Boolean(entry?.isCurrentUser),
          isPrimary: Boolean(entry?.isPrimary),
        })),
        currentUser: String(payload?.currentUser || ''),
        primaryUser: String(payload?.primaryUser || ''),
      });
    } catch {
      setTopbarAdminsData(EMPTY_TOPBAR_ADMINS_DATA);
    }
  }, []);

  const fetchTopbarSystemStatus = useCallback(async () => {
    try {
      const res = await fetch('/api/admin/status');
      if (!res.ok) {
        setTopbarSystemStatus(null);
        return;
      }

      const payload = await res.json();
      setTopbarSystemStatus(payload as AdminStatus);
    } catch {
      setTopbarSystemStatus(null);
    }
  }, []);

  const mergeCfgFormIntoJson = useCallback(
    (rawJsonText: string, form: CfgFormState): string => {
      const parsed = JSON.parse(rawJsonText) as Record<string, unknown>;
      parsed.name =
        form.serverName.trim() || parsed.name || DEFAULT_CFG_FORM.serverName;
      parsed.master = normalizeMasterUrl(form.masterUrl);
      parsed.masterKey = form.masterKey.trim();
      parsed.port = Math.max(1, Math.floor(Number(form.port) || 7777));
      parsed.maxPlayers = Math.max(
        1,
        Math.floor(Number(form.maxPlayers) || 100),
      );
      parsed.offlineMode = Boolean(form.offlineMode);
      parsed.itemPickupMode = form.itemPickupMode;
      parsed.showCharacterNicknames = Boolean(form.showCharacterNicknames);

      const localeRouting =
        parsed.localeRouting && typeof parsed.localeRouting === 'object'
          ? (parsed.localeRouting as Record<string, unknown>)
          : {};
      localeRouting.defaultLanguage = form.defaultLanguage.trim() || 'en';
      parsed.localeRouting = localeRouting;

      const normalizedStartSpawn = {
        pos: [
          Number(form.startSpawn.x) || 0,
          Number(form.startSpawn.y) || 0,
          Number(form.startSpawn.z) || 0,
        ],
        worldOrCell:
          form.startSpawn.worldOrCell.trim() ||
          DEFAULT_CFG_FORM.startSpawn.worldOrCell,
        angleZ: Number(form.startSpawn.angleZ) || 0,
      };

      parsed.startSpawn = normalizedStartSpawn;
      parsed.startPoints = [normalizedStartSpawn];

      const npcSettings =
        parsed.npcSettings && typeof parsed.npcSettings === 'object'
          ? (parsed.npcSettings as Record<string, unknown>)
          : {};
      const npcDefaultSettings =
        npcSettings.default && typeof npcSettings.default === 'object'
          ? (npcSettings.default as Record<string, unknown>)
          : {};
      npcDefaultSettings.spawnInInterior = Boolean(
        form.npcDefaultSettings.spawnInInterior,
      );
      npcDefaultSettings.spawnInExterior = Boolean(
        form.npcDefaultSettings.spawnInExterior,
      );
      npcDefaultSettings.allowHumanoid = Boolean(
        form.npcDefaultSettings.allowHumanoid,
      );
      npcDefaultSettings.allowCreature = Boolean(
        form.npcDefaultSettings.allowCreature,
      );
      npcSettings.default = npcDefaultSettings;
      parsed.npcEnabled = Boolean(form.npcEnabled);
      parsed.npcSettings = npcSettings;

      parsed.joinAccess = {
        mode: form.joinAccess.mode,
        rejectionMessage:
          form.joinAccess.rejectionMessage.trim() ||
          DEFAULT_CFG_FORM.joinAccess.rejectionMessage,
        approvedLicenses: parseCommaList(form.joinAccess.approvedLicenses),
        approvedDiscordIds: parseCommaList(form.joinAccess.approvedDiscordIds),
        discordRoleIds: parseCommaList(form.joinAccess.discordRoleIds),
      };

      parsed.discordBot = {
        enabled: form.discordBot.enabled,
        token: form.discordBot.token.trim(),
        guildId: form.discordBot.guildId.trim(),
        warningsChannelId: form.discordBot.warningsChannelId.trim(),
      };

      parsed.discordAuth = {
        ...(parsed.discordAuth && typeof parsed.discordAuth === 'object'
          ? (parsed.discordAuth as Record<string, unknown>)
          : {}),
        botToken: form.discordBot.token.trim(),
        guildId: form.discordBot.guildId.trim(),
        eventLogChannelId: form.discordBot.warningsChannelId.trim(),
      };

      parsed.starterInventory = {
        entries: form.starterInventory.map((entry) => ({
          baseId: Math.max(0, Math.floor(entry.baseId)),
          count: Math.max(1, Math.floor(entry.count)),
          ...(entry.worn ? { worn: true } : {}),
          ...(entry.wornLeft ? { wornLeft: true } : {}),
        })),
      };

      parsed.supervisor = {
        enabled: Boolean(form.supervisorEnabled),
        stopCommand: form.supervisorStopCommand.trim(),
        restartCommand: form.supervisorRestartCommand.trim(),
      };

      return JSON.stringify(parsed, null, 2);
    },
    [],
  );

  const addStarterInventoryEntry = useCallback(() => {
    const code = inventoryCustomCode.trim() || inventoryItemCode;
    const baseId = parseItemCodeToBaseId(code);
    if (baseId === null) {
      setCfgEditorStatus(t('adminDashboard.cfgInvalidItemCode'));
      return;
    }

    setCfgForm((prev) => ({
      ...prev,
      starterInventory: [
        ...prev.starterInventory,
        {
          baseId,
          count: Math.max(1, Math.floor(Number(inventoryCount) || 1)),
          worn: inventoryWorn,
          wornLeft: inventoryWornLeft,
        },
      ],
    }));
    setInventoryCustomCode('');
    setInventoryCount(1);
    setInventoryWorn(false);
    setInventoryWornLeft(false);
  }, [
    inventoryCount,
    inventoryCustomCode,
    inventoryItemCode,
    inventoryWorn,
    inventoryWornLeft,
    t,
  ]);

  const removeStarterInventoryEntry = useCallback((index: number) => {
    setCfgForm((prev) => ({
      ...prev,
      starterInventory: prev.starterInventory.filter(
        (_, currentIndex) => currentIndex !== index,
      ),
    }));
  }, []);

  const loadCfgEditor = useCallback(async () => {
    setCfgEditorLoading(true);
    try {
      const res = await fetch('/api/admin/cfg/server-settings');
      if (!res.ok) {
        if (res.status === 403) {
          setCfgEditorStatus(t('adminDashboard.noPermission'));
        } else {
          setCfgEditorStatus(t('adminDashboard.apiError'));
        }
        return;
      }
      const payload = await res.json();
      const text =
        typeof payload?.json === 'string'
          ? payload.json
          : JSON.stringify(payload ?? {}, null, 2);
      setCfgEditorText(text);
      try {
        setCfgForm(mapJsonToCfgForm(text));
      } catch {
        setCfgForm(DEFAULT_CFG_FORM);
      }
      const pathText =
        typeof payload?.path === 'string' && payload.path.length > 0
          ? `: ${payload.path}`
          : '';
      setCfgEditorStatus(`${t('adminDashboard.cfgLoaded')}${pathText}`);
    } catch {
      setCfgEditorStatus(t('adminDashboard.apiError'));
    } finally {
      setCfgEditorLoading(false);
    }
  }, [mapJsonToCfgForm, t]);

  const formatCfgEditor = useCallback(() => {
    try {
      const parsed = JSON.parse(cfgEditorText);
      const formatted = JSON.stringify(parsed, null, 2);
      setCfgEditorText(formatted);
      setCfgForm(mapJsonToCfgForm(formatted));
      setCfgEditorStatus(t('adminDashboard.cfgFormatted'));
    } catch {
      setCfgEditorStatus(t('adminDashboard.cfgInvalidJson'));
    }
  }, [cfgEditorText, mapJsonToCfgForm, t]);

  const applyAccessDiscordToCfgEditor = useCallback(() => {
    try {
      const parsed = JSON.parse(cfgEditorText);
      const next =
        parsed && typeof parsed === 'object'
          ? (parsed as Record<string, unknown>)
          : {};

      const normalizeJoinMode = (
        value: unknown,
      ): 'none' | 'approvedLicense' | 'discordMember' | 'discordRoles' => {
        const mode = String(value || '').trim();
        if (
          mode === 'approvedLicense' ||
          mode === 'discordMember' ||
          mode === 'discordRoles'
        )
          return mode;
        return 'none';
      };

      const asStringArray = (value: unknown): string[] => {
        if (!Array.isArray(value)) return [];
        return value
          .map((item) => String(item || '').trim())
          .filter((item) => item.length > 0);
      };

      const joinAccessRaw =
        next.joinAccess && typeof next.joinAccess === 'object'
          ? (next.joinAccess as Record<string, unknown>)
          : {};
      const discordBotRaw =
        next.discordBot && typeof next.discordBot === 'object'
          ? (next.discordBot as Record<string, unknown>)
          : {};
      const discordAuthRaw =
        next.discordAuth && typeof next.discordAuth === 'object'
          ? (next.discordAuth as Record<string, unknown>)
          : {};

      const joinAccess = {
        mode: normalizeJoinMode(joinAccessRaw.mode),
        rejectionMessage: String(
          joinAccessRaw.rejectionMessage ||
            'Access denied. Please contact server staff for whitelist approval.',
        ),
        approvedLicenses: asStringArray(joinAccessRaw.approvedLicenses),
        approvedDiscordIds: asStringArray(joinAccessRaw.approvedDiscordIds),
        discordRoleIds: asStringArray(joinAccessRaw.discordRoleIds),
      };

      const discordBot = {
        enabled: Boolean(discordBotRaw.enabled),
        token: String(discordBotRaw.token || discordAuthRaw.botToken || ''),
        guildId: String(discordBotRaw.guildId || discordAuthRaw.guildId || ''),
        warningsChannelId: String(
          discordBotRaw.warningsChannelId ||
            discordAuthRaw.eventLogChannelId ||
            '',
        ),
      };

      next.joinAccess = joinAccess;
      next.discordBot = discordBot;
      next.discordAuth = {
        ...discordAuthRaw,
        botToken: discordBot.token,
        guildId: discordBot.guildId,
        eventLogChannelId: discordBot.warningsChannelId,
      };

      const normalized = JSON.stringify(next, null, 2);
      setCfgEditorText(normalized);
      setCfgForm(mapJsonToCfgForm(normalized));
      setCfgEditorStatus(t('adminDashboard.accessApplied'));
      return normalized;
    } catch {
      setCfgEditorStatus(t('adminDashboard.cfgInvalidJson'));
      return null;
    }
  }, [cfgEditorText, mapJsonToCfgForm, t]);

  const saveCfgEditor = useCallback(
    async (preparedJson?: string) => {
      let normalized: string;
      try {
        normalized = mergeCfgFormIntoJson(
          preparedJson ?? cfgEditorText,
          cfgForm,
        );
      } catch {
        setCfgEditorStatus(t('adminDashboard.cfgInvalidJson'));
        return;
      }

      setCfgEditorSaving(true);
      try {
        const res = await fetch('/api/admin/cfg/server-settings', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ json: normalized }),
        });
        if (!res.ok) {
          if (res.status === 403) {
            setCfgEditorStatus(t('adminDashboard.noPermission'));
          } else {
            const text = await res.text().catch(() => '');
            setCfgEditorStatus(
              `${t('adminDashboard.apiError')}${
                text ? `: ${text.slice(0, 120)}` : ''
              }`,
            );
          }
          return;
        }
        const nextLanguage = detectLanguage(cfgForm.defaultLanguage);
        setCfgEditorText(normalized);
        setCfgForm(mapJsonToCfgForm(normalized));
        persistRuntimeLanguage(nextLanguage);
        if (i18n.resolvedLanguage !== nextLanguage) {
          await i18n.changeLanguage(nextLanguage);
        }
        setCfgEditorStatus(
          i18n.getFixedT(nextLanguage)('adminDashboard.cfgSaved'),
        );
      } catch {
        setCfgEditorStatus(t('adminDashboard.apiError'));
      } finally {
        setCfgEditorSaving(false);
      }
    },
    [cfgEditorText, cfgForm, i18n, mapJsonToCfgForm, mergeCfgFormIntoJson, t],
  );

  const revivePlayer = useCallback(
    async (userId: number) => {
      if (!isValidModerationReason(moderationReason)) {
        setStatusMsg(
          t('adminDashboard.reasonTooLong', {
            max: MAX_MODERATION_REASON_LENGTH,
            defaultValue: 'Reason is too long (max {{max}} characters).',
          }),
        );
        return;
      }

      const reason = moderationReason.trim();
      try {
        const res = await fetch('/api/admin/revive', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(reason ? { userId, reason } : { userId }),
        });
        setForbiddenAwareStatus(
          res,
          `${t('adminDashboard.revived')}: ${userId}`,
        );
        if (res.ok) {
          await fetchDownedPlayers();
          await fetchRevivalEvents();
        }
      } catch {
        setStatusMsg(t('adminDashboard.apiError'));
      }
    },
    [
      fetchDownedPlayers,
      fetchRevivalEvents,
      moderationReason,
      setForbiddenAwareStatus,
      t,
    ],
  );

  const kickPlayer = async (userId: number) => {
    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    try {
      const res = await fetch(`/api/admin/players/${userId}/kick`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(reason ? { reason } : {}),
      });
      setForbiddenAwareStatus(res, `${t('adminDashboard.kicked')}: ${userId}`);
      if (res.ok) await fetchData();
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const banPlayer = async (userId: number) => {
    if (!window.confirm(`${t('adminDashboard.banConfirm')} userId=${userId}?`))
      return;

    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    try {
      const body: Record<string, unknown> = {
        durationMinutes: banDurationMinutes,
      };
      if (reason) body.reason = reason;
      const res = await fetch(`/api/admin/players/${userId}/ban`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      });
      setForbiddenAwareStatus(res, `${t('adminDashboard.banned')}: ${userId}`);
      if (res.ok) {
        await fetchData();
        await fetchBans();
      }
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const unbanPlayer = async (userId: number) => {
    if (
      !window.confirm(`${t('adminDashboard.unbanConfirm')} userId=${userId}?`)
    )
      return;
    try {
      const res = await fetch(`/api/admin/players/${userId}/ban`, {
        method: 'DELETE',
      });
      setForbiddenAwareStatus(
        res,
        `${t('adminDashboard.unbanned')}: ${userId}`,
      );
      if (res.ok) await fetchBans();
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const sendMessageToPlayer = async () => {
    if (sendMsgTargetId === null || !capabilities.canMessage) return;
    const text = sendMsgText.trim();

    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    if (!text) return;
    setSendMsgSending(true);
    try {
      const res = await fetch(`/api/admin/players/${sendMsgTargetId}/message`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(
          reason ? { message: text, reason } : { message: text },
        ),
      });
      setForbiddenAwareStatus(res, t('adminDashboard.messageSent'));
      if (res.ok) {
        setSendMsgTargetId(null);
        setSendMsgText('');
        setModerationReason('');
      }
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setSendMsgSending(false);
    }
  };

  const openWarnDialog = (userId: number, actorName: string) => {
    setWarnTargetId(userId);
    setWarnTargetName(actorName || String(userId));
    setWarnMessage('');
    setWarnReason('');
    setWarnDialogOpen(true);
  };

  const closeWarnDialog = () => {
    setWarnDialogOpen(false);
    setWarnTargetId(null);
    setWarnTargetName('');
    setWarnMessage('');
    setWarnReason('');
    setWarnSending(false);
  };

  const sendWarn = async () => {
    if (warnTargetId === null || !capabilities.canWarn) return;
    const message = warnMessage.trim();

    if (!isValidModerationReason(warnReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = warnReason.trim();
    if (!message) {
      setStatusMsg(
        t('adminDashboard.warnMessageRequired', {
          defaultValue: 'Warn-Nachricht erforderlich.',
        }),
      );
      return;
    }
    setWarnSending(true);
    try {
      const res = await fetch(`/api/admin/players/${warnTargetId}/warn`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(reason ? { message, reason } : { message }),
      });
      setForbiddenAwareStatus(
        res,
        t('adminDashboard.warnSent', { defaultValue: 'Warnung gesendet.' }),
      );
      if (res.ok) {
        closeWarnDialog();
        await fetchTopbarHistory();
      }
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setWarnSending(false);
    }
  };

  const showPlayerInventory = async (userId: number, actorName: string) => {
    setInventoryTargetUserId(userId);
    setInventoryTargetName(actorName || String(userId));
    setInventoryError('');
    setPlayerInventory(null);
    setInventoryLoading(true);

    try {
      const res = await fetch(`/api/admin/players/${userId}/inventory`);
      const payload = await res
        .json()
        .catch(() => ({} as Record<string, unknown>));
      if (!res.ok) {
        const errorText =
          typeof payload.error === 'string'
            ? payload.error
            : t('adminDashboard.apiError');
        setInventoryError(errorText);
        return;
      }

      setPlayerInventory(payload as AdminPlayerInventorySnapshot);
    } catch {
      setInventoryError(t('adminDashboard.apiError'));
    } finally {
      setInventoryLoading(false);
    }
  };

  const kickAllPlayers = async () => {
    if (!capabilities.canKick || players.length === 0) return;
    if (
      !window.confirm(
        `${t('adminDashboard.kickAllConfirm')} (${players.length})?`,
      )
    )
      return;

    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    setSidebarActionSending('kick-all');
    try {
      const res = await fetch('/api/admin/players/kick-all', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(reason ? { reason } : {}),
      });
      setForbiddenAwareStatus(
        res,
        `${t('adminDashboard.kickAllPlayers')}: ${players.length}`,
      );
      if (res.ok) {
        await fetchData();
      }
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setSidebarActionSending(null);
    }
  };

  const runServerControl = async (action: 'stop' | 'restart') => {
    if (!capabilities.canConsole) {
      setStatusMsg(t('adminDashboard.noPermission'));
      return;
    }

    if (!serverControlAvailable) {
      setStatusMsg(
        t('adminDashboard.serverControlUnavailable', {
          defaultValue: 'Server control requires a configured supervisor.',
        }),
      );
      return;
    }

    const confirmMessage =
      action === 'stop'
        ? t('adminDashboard.stopServer')
        : t('adminDashboard.restartServer');

    if (!window.confirm(`${confirmMessage}?`)) return;

    setSidebarActionSending(
      action === 'stop' ? 'server-stop' : 'server-restart',
    );
    try {
      const res = await fetch('/api/admin/server/control', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action }),
      });

      if (res.status === 403) {
        setStatusMsg(t('adminDashboard.noPermission'));
        return;
      }

      if (res.status === 409) {
        const payload = await res.json().catch(() => null);
        setStatusMsg(
          typeof payload?.error === 'string'
            ? payload.error
            : t('adminDashboard.serverControlUnavailable', {
                defaultValue:
                  'Server control requires a configured supervisor.',
              }),
        );
        return;
      }

      if (!res.ok) {
        setStatusMsg(t('adminDashboard.apiError'));
        return;
      }

      setStatusMsg(
        action === 'stop'
          ? t('adminDashboard.stopServer')
          : t('adminDashboard.restartServer'),
      );
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setSidebarActionSending(null);
    }
  };

  const sendAnnouncement = async () => {
    if (!capabilities.canMessage) return;
    const message =
      window.prompt(t('adminDashboard.announcementPrompt'))?.trim() ?? '';
    if (!message) return;

    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    setSidebarActionSending('announcement');
    try {
      const body: Record<string, unknown> = { message };
      if (reason) body.reason = reason;
      const res = await fetch('/api/admin/announcement', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(body),
      });
      setForbiddenAwareStatus(res, t('adminDashboard.announcementSent'));
      if (res.ok) {
        setModerationReason('');
      }
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setSidebarActionSending(null);
    }
  };

  const mutePlayer = async (userId: number) => {
    if (!capabilities.canMute) return;

    if (!isValidModerationReason(moderationReason)) {
      setStatusMsg(
        t('adminDashboard.reasonTooLong', {
          max: MAX_MODERATION_REASON_LENGTH,
          defaultValue: 'Reason is too long (max {{max}} characters).',
        }),
      );
      return;
    }

    const reason = moderationReason.trim();
    try {
      const res = await fetch(`/api/admin/players/${userId}/mute`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(
          reason
            ? { durationMinutes: muteDurationMinutes, reason }
            : { durationMinutes: muteDurationMinutes },
        ),
      });
      setForbiddenAwareStatus(res, `${t('adminDashboard.muted')}: ${userId}`);
      if (res.ok) await fetchMutes();
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const unmutePlayer = async (userId: number) => {
    if (
      !window.confirm(`${t('adminDashboard.unmuteConfirm')} userId=${userId}?`)
    )
      return;
    try {
      const res = await fetch(`/api/admin/players/${userId}/mute`, {
        method: 'DELETE',
      });
      setForbiddenAwareStatus(res, `${t('adminDashboard.unmuted')}: ${userId}`);
      if (res.ok) await fetchMutes();
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const sendConsoleCommand = async () => {
    if (!capabilities.canConsole) return;

    const cmd = consoleInput.trim();
    if (!cmd) return;

    setConsoleHistory((prev) => {
      const next = [cmd, ...prev.filter((entry) => entry !== cmd)];
      return next.slice(0, 30);
    });
    setConsoleHistoryIndex(null);

    setConsoleLines((prev) => [...prev, { text: `> ${cmd}`, kind: 'input' }]);
    setConsoleInput('');
    setConsoleSending(true);

    try {
      const res = await fetch('/api/admin/console', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command: cmd }),
      });

      const payload = await res
        .json()
        .catch(() => ({} as Record<string, unknown>));
      const resultText =
        typeof payload.resultText === 'string' ? payload.resultText : '';
      const errorText = typeof payload.error === 'string' ? payload.error : '';

      if (res.ok) {
        const msg = resultText
          ? `${t('adminDashboard.consoleResult')}: ${resultText}`
          : t('adminDashboard.consoleSent');
        setConsoleLines((prev) => [...prev, { text: msg, kind: 'ok' }]);
      } else if (res.status === 403) {
        setConsoleLines((prev) => [
          ...prev,
          { text: t('adminDashboard.noPermission'), kind: 'err' },
        ]);
      } else {
        const msg = errorText
          ? `${t('adminDashboard.consoleError')}: ${errorText}`
          : t('adminDashboard.apiError');
        setConsoleLines((prev) => [...prev, { text: msg, kind: 'err' }]);
      }
    } catch {
      setConsoleLines((prev) => [
        ...prev,
        { text: t('adminDashboard.apiError'), kind: 'err' },
      ]);
    } finally {
      setConsoleSending(false);
    }
  };

  const handleConsoleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      void sendConsoleCommand();
      return;
    }

    if (e.key === 'ArrowUp') {
      e.preventDefault();
      if (consoleHistory.length === 0) return;
      const nextIndex =
        consoleHistoryIndex === null
          ? 0
          : Math.min(consoleHistoryIndex + 1, consoleHistory.length - 1);
      setConsoleHistoryIndex(nextIndex);
      setConsoleInput(consoleHistory[nextIndex]);
      return;
    }

    if (e.key === 'ArrowDown') {
      e.preventDefault();
      if (consoleHistoryIndex === null) return;
      const nextIndex = consoleHistoryIndex - 1;
      if (nextIndex < 0) {
        setConsoleHistoryIndex(null);
        setConsoleInput('');
      } else {
        setConsoleHistoryIndex(nextIndex);
        setConsoleInput(consoleHistory[nextIndex]);
      }
    }
  };

  // WebSocket for Status + Players updates
  const wsStatusState = useAdminWebSocketMessages('status', (data) => {
    if (status) {
      setStatus({
        ...status,
        name: data.name || status.name,
        master: data.master || status.master,
        online: data.online !== undefined ? data.online : status.online,
        maxPlayers: data.maxPlayers || status.maxPlayers,
        port: data.port || status.port,
        itemPickupMode: data.itemPickupMode || status.itemPickupMode,
        uptimeSec: data.uptimeSec !== undefined ? data.uptimeSec : status.uptimeSec,
      });
    }
    setLastUpdated(new Date().toLocaleTimeString());
  }, visible);

  const wsPlayersState = useAdminWebSocketMessages('players', (data) => {
    if (data.players && Array.isArray(data.players)) {
      setPlayers(data.players);
      setLastUpdated(new Date().toLocaleTimeString());
    }
  }, visible);

  // WebSocket for Logs stream (append new entries only)
  const wsLogState = useAdminWebSocketMessages('log', (data) => {
    if (data.entry && capabilities.canViewLogs) {
      setLogEntries((prev) => {
        // Avoid duplicates
        if (
          prev.length > 0 &&
          prev[0].ts === data.entry.ts &&
          prev[0].message === data.entry.message
        ) {
          return prev;
        }
        // Prepend new entry and keep last 200
        return [data.entry, ...prev.slice(0, 199)];
      });
    }
  }, visible && activeTab === 'logs');

  // WebSocket for Events stream (append new entries only)
  const wsEventState = useAdminWebSocketMessages('event', (data) => {
    if (data.entry && capabilities.canViewLogs) {
      setRevivalEvents((prev) => {
        // Avoid duplicates
        if (
          prev.length > 0 &&
          prev[0].ts === data.entry.ts &&
          prev[0].type === data.entry.type &&
          prev[0].userId === data.entry.userId &&
          prev[0].actorName === data.entry.actorName &&
          prev[0].details === data.entry.details
        ) {
          return prev;
        }
        // Prepend new event and keep last 100
        return [data.entry, ...prev.slice(0, 99)];
      });
    }
  }, visible && activeTab === 'events');

  // WebSocket for Respawn status updates
  const wsRespawnState = useAdminWebSocketMessages('respawn', (data) => {
    if (data.players && Array.isArray(data.players) && capabilities.canManageRespawn) {
      setDownedPlayers(data.players);
    }
  }, visible && activeTab === 'respawn');

  // WebSocket for History entries (append new entries only)
  const wsHistoryState = useAdminWebSocketMessages('history', (data) => {
    if (data.entry && capabilities.canViewLogs) {
      setTopbarHistoryData((prev) => {
        // Avoid duplicates
        if (
          prev.entries.length > 0 &&
          prev.entries[0].id === data.entry.id
        ) {
          return prev;
        }
        // Prepend new history entry and update summary counts
        return {
          ...prev,
          entries: [data.entry, ...prev.entries].slice(0, 200),
          totalWarns: data.totalWarns !== undefined ? data.totalWarns : prev.totalWarns,
          newWarns7d: data.newWarns7d !== undefined ? data.newWarns7d : prev.newWarns7d,
          totalBans: data.totalBans !== undefined ? data.totalBans : prev.totalBans,
          newBans7d: data.newBans7d !== undefined ? data.newBans7d : prev.newBans7d,
        };
      });
    }
  }, visible && activeTopSection === 'history');

  const wsStates = useMemo(
    () => [
      wsStatusState,
      wsPlayersState,
      wsLogState,
      wsEventState,
      wsRespawnState,
      wsHistoryState,
    ],
    [
      wsEventState,
      wsHistoryState,
      wsLogState,
      wsPlayersState,
      wsRespawnState,
      wsStatusState,
    ],
  );
  const wsAnyConnected = useMemo(
    () => wsStates.some((state) => state.connected),
    [wsStates],
  );
  const wsAnyReconnecting = useMemo(
    () => wsStates.some((state) => state.reconnecting),
    [wsStates],
  );
  const wsTotalMessages = useMemo(
    () => wsStates.reduce((sum, state) => sum + state.messageCount, 0),
    [wsStates],
  );
  const wsLastMessageTs = useMemo(
    () =>
      wsStates.reduce<number>(
        (maxTs, state) => Math.max(maxTs, state.lastMessageTs ?? 0),
        0,
      ),
    [wsStates],
  );
  const wsLastMessageLabel = useMemo(
    () => (wsLastMessageTs > 0 ? formatAdminTime(wsLastMessageTs) : '-'),
    [wsLastMessageTs],
  );
  const wsHealthTone = wsAnyConnected
    ? 'live'
    : wsAnyReconnecting
      ? 'reconnecting'
      : 'fallback';
  const wsHealthLabel = wsAnyConnected
    ? t('adminDashboard.wsHealthLive', { defaultValue: 'WS live' })
    : wsAnyReconnecting
      ? t('adminDashboard.wsHealthReconnecting', {
          defaultValue: 'WS reconnecting',
        })
      : t('adminDashboard.wsHealthFallback', {
          defaultValue: 'HTTP fallback',
        });

  const wsHadConnectedRef = useRef(false);
  const wsPrevConnectedRef = useRef(false);
  const wsPrevReconnectingRef = useRef(false);
  const [wsReconnectCount, setWsReconnectCount] = useState(0);
  const [wsReconnectEvents, setWsReconnectEvents] = useState<WsReconnectEvent[]>(() => {
    try {
      const raw = window.localStorage.getItem(ADMIN_WS_RECONNECT_HISTORY_KEY);
      if (!raw) return [];
      const parsed = JSON.parse(raw);
      if (!Array.isArray(parsed)) return [];
      return parsed
        .filter(
          (entry): entry is WsReconnectEvent =>
            entry &&
            typeof entry.ts === 'number' &&
            (entry.kind === 'reconnected' ||
              entry.kind === 'reconnecting' ||
              entry.kind === 'fallback'),
        )
        .slice(0, WS_RECONNECT_HISTORY_LIMIT);
    } catch {
      return [];
    }
  });
  const appendWsReconnectEvent = useCallback((kind: WsReconnectEventKind) => {
    setWsReconnectEvents((prev) =>
      [{ ts: Date.now(), kind }, ...prev].slice(0, WS_RECONNECT_HISTORY_LIMIT),
    );
  }, []);

  useEffect(() => {
    if (!visible) {
      wsPrevConnectedRef.current = false;
      wsPrevReconnectingRef.current = false;
      return;
    }

    if (
      wsAnyConnected &&
      wsHadConnectedRef.current &&
      !wsPrevConnectedRef.current
    ) {
      setWsReconnectCount((prev) => prev + 1);
      appendWsReconnectEvent('reconnected');
    }

    if (wsAnyReconnecting && !wsPrevReconnectingRef.current) {
      appendWsReconnectEvent('reconnecting');
    }

    if (!wsAnyConnected && !wsAnyReconnecting && wsPrevConnectedRef.current) {
      appendWsReconnectEvent('fallback');
    }

    if (wsAnyConnected) {
      wsHadConnectedRef.current = true;
    }
    wsPrevConnectedRef.current = wsAnyConnected;
    wsPrevReconnectingRef.current = wsAnyReconnecting;
  }, [appendWsReconnectEvent, visible, wsAnyConnected, wsAnyReconnecting]);

  useEffect(() => {
    try {
      window.localStorage.setItem(
        ADMIN_WS_RECONNECT_HISTORY_KEY,
        JSON.stringify(wsReconnectEvents),
      );
    } catch {
      // ignore storage write failures
    }
  }, [wsReconnectEvents]);

  const show = useCallback(async () => {
    setVisible(true);
    setLoading(true);
    await Promise.all([
      fetchData(),
      fetchCapabilities(),
      fetchBans(),
      fetchUpdateStatus(),
    ]);
    setLoading(false);
  }, [fetchBans, fetchCapabilities, fetchData, fetchUpdateStatus]);

  const hide = useCallback(() => {
    setVisible(false);
    setStatus(null);
    setPlayers([]);
    setBannedUsers([]);
    setMutedUsers([]);
    setStatusMsg('');
    setConsoleLines([]);
    setConsoleHistoryIndex(null);
    setServerConsoleEntries([]);
    setLogEntries([]);
    setLogBeforeTs(null);
    setLogHasMore(false);
    setMetricEntries([]);
    setMetricSummary(EMPTY_FRONTEND_METRICS_SUMMARY);
    setClientRuntimeEntries([]);
    setClientRuntimeSummary(EMPTY_CLIENT_RUNTIME_SUMMARY);
    setDownedPlayers([]);
    setRevivalEvents([]);
    setTopbarHistoryData({
      entries: [],
      totalWarns: 0,
      newWarns7d: 0,
      totalBans: 0,
      newBans7d: 0,
      admins: [],
    });
    setTopbarPlayerDropsData(EMPTY_PLAYER_DROPS_DATA);
    setTopbarWhitelistSnapshot(EMPTY_TOPBAR_WHITELIST);
    setTopbarWhitelistSearch('');
    setTopbarAdminSnapshot(EMPTY_TOPBAR_ADMIN);
    setTopbarAdminsData(EMPTY_TOPBAR_ADMINS_DATA);
    setTopbarAdminModalOpen(false);
    setTopbarAdminEditingUser(null);
    setTopbarAdminForm(EMPTY_TOPBAR_ADMIN_FORM);
    setTopbarAdminSaving(false);
    setTopbarSystemStatus(null);
    setUpdateStatus(EMPTY_ADMIN_UPDATE_STATUS);
    setUpdateModalOpen(false);
    setSendMsgTargetId(null);
    setSendMsgText('');
    setModerationReason('');
    if (intervalRef.current) {
      clearInterval(intervalRef.current);
      intervalRef.current = null;
    }
  }, []);

  // Manage polling fallback based on WebSocket connection state
  useEffect(() => {
    if (!visible) {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
      return;
    }

    if (wsStatusState.connected) {
      // WebSocket is primary - clear polling interval
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    } else {
      // WebSocket unavailable - use polling fallback
      if (!intervalRef.current) {
        intervalRef.current = setInterval(fetchData, REFRESH_INTERVAL_MS);
      }
    }

    return () => {
      if (intervalRef.current) {
        clearInterval(intervalRef.current);
        intervalRef.current = null;
      }
    };
  }, [visible, wsStatusState.connected, fetchData]);

  useEffect(() => {
    const onShow = () => void show();
    const onHide = () => hide();

    window.addEventListener('showAdminDashboard', onShow);
    window.addEventListener('hideAdminDashboard', onHide);

    // Dev/test shortcut: open admin directly via URL (?admin=1 or #admin)
    const params = new URLSearchParams(window.location.search);
    const shouldAutoOpen =
      params.get('admin') === '1' || window.location.hash === '#admin';
    if (shouldAutoOpen) {
      void show();
    }

    return () => {
      window.removeEventListener('showAdminDashboard', onShow);
      window.removeEventListener('hideAdminDashboard', onHide);
      if (intervalRef.current) clearInterval(intervalRef.current);
    };
  }, [hide, show]);

  useEffect(() => {
    if (!visible) return;

    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key === 'Escape') {
        event.preventDefault();
        hide();
      }
    };

    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, [hide, visible]);

  useEffect(() => {
    window.dispatchEvent(
      new CustomEvent('adminDashboardVisibility', { detail: { visible } }),
    );
  }, [visible]);

  useEffect(() => {
    if (consoleEndRef.current)
      consoleEndRef.current.scrollIntoView({ behavior: 'smooth' });
  }, [consoleLines, serverConsoleEntries]);

  useEffect(() => {
    if (!visible || activeMenuSurface !== 'sidebar') return;

    if (activeTab === 'logs') void fetchLogs();
    if (activeTab === 'metrics') void fetchFrontendMetrics();
    if (activeTab === 'metrics') void fetchClientRuntimeEvents();
    if (activeTab === 'respawn') void fetchDownedPlayers();
    if (activeTab === 'events') void fetchRevivalEvents();
    if (activeTab === 'cfg') void loadCfgEditor();
  }, [
    activeMenuSurface,
    activeTab,
    fetchClientRuntimeEvents,
    fetchDownedPlayers,
    fetchFrontendMetrics,
    fetchLogs,
    fetchRevivalEvents,
    loadCfgEditor,
    visible,
  ]);

  // Periodic polling fallback for Logs+Events when WebSocket unavailable
  useEffect(() => {
    if (!visible || activeMenuSurface !== 'sidebar') return;

    let logsInterval: NodeJS.Timeout | null = null;
    let eventsInterval: NodeJS.Timeout | null = null;
    let respawnInterval: NodeJS.Timeout | null = null;

    // Logs polling fallback (5s interval when WebSocket unavailable)
    if (activeTab === 'logs' && !wsLogState.connected && capabilities.canViewLogs) {
      logsInterval = setInterval(() => {
        void fetchLogs();
      }, 5000);
    }

    // Events polling fallback (5s interval when WebSocket unavailable)
    if (activeTab === 'events' && !wsEventState.connected && capabilities.canViewLogs) {
      eventsInterval = setInterval(() => {
        void fetchRevivalEvents();
      }, 5000);
    }

    // Respawn polling fallback (3s interval when WebSocket unavailable)
    if (activeTab === 'respawn' && !wsRespawnState.connected && capabilities.canManageRespawn) {
      respawnInterval = setInterval(() => {
        void fetchDownedPlayers();
      }, 3000);
    }

    return () => {
      if (logsInterval) clearInterval(logsInterval);
      if (eventsInterval) clearInterval(eventsInterval);
      if (respawnInterval) clearInterval(respawnInterval);
    };
  }, [
    visible,
    activeMenuSurface,
    activeTab,
    wsLogState.connected,
    wsEventState.connected,
    wsRespawnState.connected,
    fetchLogs,
    fetchRevivalEvents,
    fetchDownedPlayers,
    capabilities.canViewLogs,
    capabilities.canManageRespawn,
  ]);

  // Periodic polling fallback for History when WebSocket unavailable
  useEffect(() => {
    if (!visible || activeMenuSurface !== 'topbar') return;

    let historyInterval: NodeJS.Timeout | null = null;

    // History polling fallback (5s interval when WebSocket unavailable)
    if (activeTopSection === 'history' && !wsHistoryState.connected && capabilities.canViewLogs) {
      historyInterval = setInterval(() => {
        void fetchTopbarHistory();
      }, 5000);
    }

    return () => {
      if (historyInterval) clearInterval(historyInterval);
    };
  }, [
    visible,
    activeMenuSurface,
    activeTopSection,
    wsHistoryState.connected,
    fetchTopbarHistory,
    capabilities.canViewLogs,
  ]);

  useEffect(() => {
    if (!visible || activeMenuSurface !== 'topbar') return;

    if (activeTopSection === 'history') {
      void fetchTopbarHistory();
      return;
    }

    if (activeTopSection === 'playerDrops') {
      void fetchTopbarPlayerDrops(
        dropsHoursWindow,
        crashReasonsLimit,
        crashReasonsSortMode,
      );
      return;
    }

    if (activeTopSection === 'whitelist' || activeTopSection === 'settings') {
      void fetchTopbarCfgSnapshots();
      return;
    }

    if (activeTopSection === 'admins') {
      void Promise.all([fetchTopbarAdminSnapshot(), fetchTopbarAdmins()]);
      return;
    }

    if (activeTopSection === 'system') {
      void fetchTopbarSystemStatus();
    }
  }, [
    activeMenuSurface,
    activeTopSection,
    crashReasonsLimit,
    crashReasonsSortMode,
    dropsHoursWindow,
    fetchTopbarAdminSnapshot,
    fetchTopbarAdmins,
    fetchTopbarCfgSnapshots,
    fetchTopbarHistory,
    fetchTopbarPlayerDrops,
    fetchTopbarSystemStatus,
    visible,
  ]);

  useEffect(() => {
    if (!visible || activeMenuSurface !== 'topbar') return;
    const id = setInterval(() => {
      if (activeTopSection === 'history' && !wsHistoryState.connected) {
        void fetchTopbarHistory();
      }
      if (activeTopSection === 'playerDrops') {
        void fetchTopbarPlayerDrops(
          dropsHoursWindow,
          crashReasonsLimit,
          crashReasonsSortMode,
        );
        setNowTs(Date.now());
      }
      if (activeTopSection === 'whitelist' || activeTopSection === 'settings') {
        void fetchTopbarCfgSnapshots();
      }
      if (activeTopSection === 'admins') {
        void Promise.all([fetchTopbarAdminSnapshot(), fetchTopbarAdmins()]);
      }
      if (activeTopSection === 'system') {
        void fetchTopbarSystemStatus();
      }
    }, 3000);
    return () => clearInterval(id);
  }, [
    activeMenuSurface,
    activeTopSection,
    crashReasonsLimit,
    crashReasonsSortMode,
    dropsHoursWindow,
    fetchTopbarAdminSnapshot,
    fetchTopbarAdmins,
    fetchTopbarCfgSnapshots,
    fetchTopbarHistory,
    fetchTopbarPlayerDrops,
    fetchTopbarSystemStatus,
    wsHistoryState.connected,
    visible,
  ]);

  useEffect(() => {
    setLogBeforeTs(null);
  }, [logLevelFilter, logTypeFilter, logLimit, logSinceMinutes]);

  useEffect(() => {
    if (
      visible &&
      activeMenuSurface === 'sidebar' &&
      activeTab === 'players' &&
      (capabilities.canBan ||
        capabilities.canUnban ||
        capabilities.canMute ||
        capabilities.canUnmute)
    ) {
      void fetchBans();
      void fetchMutes();
    }
  }, [
    activeMenuSurface,
    activeTab,
    capabilities.canBan,
    capabilities.canMute,
    capabilities.canUnban,
    capabilities.canUnmute,
    fetchBans,
    fetchMutes,
    visible,
  ]);

  useEffect(() => {
    if (
      !visible ||
      activeMenuSurface !== 'sidebar' ||
      activeTab !== 'players' ||
      !(capabilities.canMute || capabilities.canUnmute)
    )
      return;
    const id = setInterval(() => {
      void fetchMutes();
    }, 15000);
    return () => clearInterval(id);
  }, [
    activeMenuSurface,
    activeTab,
    capabilities.canMute,
    capabilities.canUnmute,
    fetchMutes,
    visible,
  ]);

  const hasTimedBans = bannedUsers.some((b) => !b.permanent);
  useEffect(() => {
    if (
      !visible ||
      activeMenuSurface !== 'sidebar' ||
      activeTab !== 'players' ||
      (mutedUsers.length === 0 && !hasTimedBans)
    )
      return;
    const id = setInterval(() => {
      setNowTs(Date.now());
    }, 1000);
    return () => clearInterval(id);
  }, [activeMenuSurface, activeTab, hasTimedBans, mutedUsers.length, visible]);

  const metricSourceOptions = useMemo(
    () => metricSummary.sources.map((item) => item.name),
    [metricSummary.sources],
  );
  const filteredCatalogItems = useMemo(
    () => ITEM_CATALOG.filter((item) => item.category === inventoryCategory),
    [inventoryCategory],
  );
  const selectedSpawnPresetKey = useMemo(
    () =>
      SPAWN_PRESETS.find((preset) =>
        matchesSpawnPreset(cfgForm.startSpawn, preset),
      )?.key ?? '',
    [cfgForm.startSpawn],
  );
  const selectedWorldOrCellValue = useMemo(() => {
    const normalizedValue = normalizeWorldOrCell(
      cfgForm.startSpawn.worldOrCell,
    );
    return WORLD_OR_CELL_OPTIONS.some(
      (option) => option.value === normalizedValue,
    )
      ? normalizedValue
      : '__custom__';
  }, [cfgForm.startSpawn.worldOrCell]);
  const worldOrCellHintKey = useMemo(
    () =>
      WORLD_OR_CELL_HINTS[
        normalizeWorldOrCell(cfgForm.startSpawn.worldOrCell)
      ] ?? 'cfgWorldOrCellHint_custom',
    [cfgForm.startSpawn.worldOrCell],
  );
  const catalogNameByBaseId = useMemo(
    () =>
      new Map(
        ITEM_CATALOG.map(
          (item) => [parseInt(item.codeHex, 16), item.name] as const,
        ),
      ),
    [],
  );

  useEffect(() => {
    if (filteredCatalogItems.length === 0) return;
    setInventoryItemCode(filteredCatalogItems[0].codeHex);
  }, [filteredCatalogItems]);

  const filteredPlayers = filterAdminPlayers(players, playerSearch);
  const isPlayerOnline = useCallback(
    (player: AdminPlayer): boolean =>
      player.online === true || Number(player.actorId) > 0,
    [],
  );
  const filteredTopbarPlayers = useMemo(() => {
    const needle = topbarPlayersSearch.trim().toLowerCase();
    const source =
      topbarPlayersFilter === 'online'
        ? players.filter((player) => isPlayerOnline(player))
        : topbarPlayersFilter === 'offline'
          ? players.filter((player) => !isPlayerOnline(player))
          : players;
    if (!needle) return source;

    if (topbarPlayersSearchMode === 'id') {
      return source.filter((player) => String(player.userId).includes(needle));
    }

    return source.filter((player) => {
      const name = String(player.actorName || '').toLowerCase();
      return name.includes(needle);
    });
  }, [
    isPlayerOnline,
    players,
    topbarPlayersFilter,
    topbarPlayersSearch,
    topbarPlayersSearchMode,
  ]);
  const nowMinus24h = useMemo(
    () => Date.now() - 24 * 60 * 60 * 1000,
    [lastUpdated],
  );
  const topbarNewPlayers24h = useMemo(
    () =>
      filteredTopbarPlayers.filter(
        (player) =>
          Number.isFinite(player.firstJoinedAt) &&
          Number(player.firstJoinedAt) >= nowMinus24h,
      ).length,
    [filteredTopbarPlayers, nowMinus24h],
  );

  const filteredHistory = useMemo(() => {
    const needle = historySearch.trim().toLowerCase();
    let result = topbarHistoryData.entries;
    if (historyActionType !== 'any') {
      result = result.filter((e) => e.type === historyActionType);
    }
    if (historyAdmin !== 'any') {
      result = result.filter(
        (e) => e.author.toLowerCase() === historyAdmin.toLowerCase(),
      );
    }
    if (needle) {
      if (historySearchMode === 'actionId') {
        result = result.filter((e) => e.id.toLowerCase().includes(needle));
      } else if (historySearchMode === 'player') {
        result = result.filter(
          (e) =>
            e.playerName.toLowerCase().includes(needle) ||
            String(e.userId).includes(needle),
        );
      } else if (historySearchMode === 'reason') {
        result = result.filter((e) => e.reason.toLowerCase().includes(needle));
      }
    }
    return result;
  }, [
    topbarHistoryData.entries,
    historySearch,
    historySearchMode,
    historyActionType,
    historyAdmin,
  ]);
  const whitelistModeLabel = useMemo(() => {
    if (topbarWhitelistSnapshot.mode === 'approvedLicense')
      return t('adminDashboard.cfgJoinMode_approvedLicense', {
        defaultValue: 'Approved License',
      });
    if (topbarWhitelistSnapshot.mode === 'discordMember')
      return t('adminDashboard.cfgJoinMode_discordMember', {
        defaultValue: 'Discord Member',
      });
    if (topbarWhitelistSnapshot.mode === 'discordRoles')
      return t('adminDashboard.cfgJoinMode_discordRoles', {
        defaultValue: 'Discord Roles',
      });
    return t('adminDashboard.cfgJoinMode_none', { defaultValue: 'Disabled' });
  }, [t, topbarWhitelistSnapshot.mode]);
  const whitelistApprovedEntries = useMemo(() => {
    const merged = [
      ...topbarWhitelistSnapshot.approvedLicenses.map((value, index) => ({
        id: `license-${index}-${value}`,
        source: 'license' as const,
        value,
      })),
      ...topbarWhitelistSnapshot.approvedDiscordIds.map((value, index) => ({
        id: `discord-${index}-${value}`,
        source: 'discord' as const,
        value,
      })),
    ];

    const needle = topbarWhitelistSearch.trim().toLowerCase();
    if (!needle) return merged;
    return merged.filter((entry) => entry.value.toLowerCase().includes(needle));
  }, [
    topbarWhitelistSearch,
    topbarWhitelistSnapshot.approvedDiscordIds,
    topbarWhitelistSnapshot.approvedLicenses,
  ]);
  const whitelistPendingJoinCount = useMemo(
    () =>
      topbarWhitelistSnapshot.approvedLicensesCount +
      topbarWhitelistSnapshot.approvedDiscordIdsCount,
    [
      topbarWhitelistSnapshot.approvedDiscordIdsCount,
      topbarWhitelistSnapshot.approvedLicensesCount,
    ],
  );
  const whitelistNotInApprovedLicenseMode =
    topbarWhitelistSnapshot.mode !== 'approvedLicense';
  const formatPeriodTs = useCallback((ts: number) => {
    const date = new Date(ts);
    const hh = String(date.getHours()).padStart(2, '0');
    const mm = String(date.getMinutes()).padStart(2, '0');
    const dd = String(date.getDate()).padStart(2, '0');
    const mon = String(date.getMonth() + 1).padStart(2, '0');
    const yy = String(date.getFullYear()).slice(-2);
    return `${hh}:${mm} - ${dd}.${mon}.${yy}`;
  }, []);
  const oldestLogTs =
    logEntries.length > 0 ? logEntries[logEntries.length - 1].ts : null;
  const activeMutedUsers = useMemo(
    () =>
      mutedUsers
        .filter((entry) => entry.expiresAt > nowTs)
        .sort((left, right) => left.expiresAt - right.expiresAt),
    [mutedUsers, nowTs],
  );
  const activeMuteByUserId = useMemo(
    () =>
      new Map(activeMutedUsers.map((entry) => [entry.userId, entry] as const)),
    [activeMutedUsers],
  );
  const activeBannedUsers = useMemo(
    () =>
      bannedUsers
        .filter(
          (entry) =>
            entry.permanent ||
            (entry.expiresAt !== null && entry.expiresAt > nowTs),
        )
        .sort((a, b) => {
          if (a.permanent !== b.permanent) return a.permanent ? 1 : -1; // timed first
          if (!a.permanent && !b.permanent)
            return (a.expiresAt ?? 0) - (b.expiresAt ?? 0);
          return a.userId - b.userId;
        }),
    [bannedUsers, nowTs],
  );
  const activeBanByUserId = useMemo(
    () =>
      new Map(activeBannedUsers.map((entry) => [entry.userId, entry] as const)),
    [activeBannedUsers],
  );

  const openOlderLogs = () => {
    if (!oldestLogTs) return;
    setLogBeforeTs(oldestLogTs);
  };

  const openRecentLogs = () => {
    setLogBeforeTs(null);
  };

  const openTopbarAdminCreateModal = () => {
    setTopbarAdminEditingUser(null);
    setTopbarAdminForm(EMPTY_TOPBAR_ADMIN_FORM);
    setTopbarAdminModalOpen(true);
  };

  const openTopbarAdminEditModal = (entry: TopbarAdminUserEntry) => {
    setTopbarAdminEditingUser(entry.user);
    setTopbarAdminForm({
      user: entry.user,
      role: entry.role,
      discordId: entry.discordId,
    });
    setTopbarAdminModalOpen(true);
  };

  const saveTopbarAdminUser = async () => {
    const user = topbarAdminForm.user.trim();
    const discordId = topbarAdminForm.discordId.trim();
    if (!user) {
      setStatusMsg(
        t('adminDashboard.topAdmins_userRequired', {
          defaultValue: 'Username is required.',
        }),
      );
      return;
    }

    if (!isValidDiscordSnowflake(discordId)) {
      setStatusMsg(
        t('adminDashboard.topAdmins_discordIdInvalid', {
          defaultValue: 'Discord ID must contain digits only.',
        }),
      );
      return;
    }

    setTopbarAdminSaving(true);
    try {
      const res = await fetch('/api/admin/admin-users', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          user,
          role: topbarAdminForm.role,
          discordId,
        }),
      });

      const payload = await res.json().catch(() => null);
      if (!res.ok) {
        if (payload?.error) {
          setStatusMsg(String(payload.error));
        } else {
          setForbiddenAwareStatus(res, '');
        }
        return;
      }

      setStatusMsg(
        topbarAdminEditingUser
          ? t('adminDashboard.topAdmins_updated', {
              defaultValue: 'Admin updated.',
            })
          : t('adminDashboard.topAdmins_added', {
              defaultValue: 'Admin added.',
            }),
      );
      setTopbarAdminModalOpen(false);
      await Promise.all([fetchTopbarAdminSnapshot(), fetchTopbarAdmins()]);
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setTopbarAdminSaving(false);
    }
  };

  const saveTopbarWhitelistEntry = async () => {
    const identifier = topbarWhitelistModalIdentifier.trim();
    if (!identifier) {
      setStatusMsg(
        t('adminDashboard.topWhitelist_identifierRequired', {
          defaultValue: 'Identifier is required.',
        }),
      );
      return;
    }

    if (!isValidWhitelistIdentifier(identifier)) {
      setStatusMsg(
        t('adminDashboard.topWhitelist_identifierInvalid', {
          defaultValue:
            'Identifier must use the format "type:value" with a supported type such as discord, steam or license.',
        }),
      );
      return;
    }

    setTopbarWhitelistModalSaving(true);
    try {
      const res = await fetch('/api/admin/whitelist/add', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ identifier }),
      });

      const payload = await res.json().catch(() => null);
      if (!res.ok) {
        if (payload?.error) {
          setStatusMsg(String(payload.error));
        } else {
          setForbiddenAwareStatus(res, '');
        }
        return;
      }

      setStatusMsg(
        t('adminDashboard.topWhitelist_added', {
          defaultValue: 'Player added to whitelist.',
        }),
      );
      setTopbarWhitelistModalOpen(false);
      await fetchTopbarCfgSnapshots();
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    } finally {
      setTopbarWhitelistModalSaving(false);
    }
  };

  const deleteTopbarAdminUser = async (user: string) => {
    if (
      !window.confirm(
        `${t('adminDashboard.topAdmins_deleteConfirm', {
          defaultValue: 'Delete admin user',
        })}: ${user}?`,
      )
    )
      return;

    try {
      const res = await fetch(
        `/api/admin/admin-users/${encodeURIComponent(user)}`,
        { method: 'DELETE' },
      );
      const payload = await res.json().catch(() => null);
      if (!res.ok) {
        if (payload?.error) {
          setStatusMsg(String(payload.error));
        } else {
          setForbiddenAwareStatus(res, '');
        }
        return;
      }
      setStatusMsg(
        t('adminDashboard.topAdmins_deleted', {
          defaultValue: 'Admin deleted.',
        }),
      );
      await Promise.all([fetchTopbarAdminSnapshot(), fetchTopbarAdmins()]);
    } catch {
      setStatusMsg(t('adminDashboard.apiError'));
    }
  };

  const resetDashboardPreferences = () => {
    setActiveMenuSurface('sidebar');
    setActiveTab('overview');
    setActiveTopSection('settings');
    setPlayerSearch('');
    setConsoleSearch('');
    setLogTypeFilter('');
    setLogLevelFilter('');
    setLogTextFilter('');
    setLogLimit(100);
    setLogSinceMinutes('');
    setLogBeforeTs(null);
    setMetricLimit(50);
    setMetricSourceFilter('');
    setMetricNameFilter('');
    setConsoleHistory([]);
    setConsoleHistoryIndex(null);
    setCfgEditorStatus('');
    setCfgEditorTab('general');
    setStatusMsg(t('adminDashboard.preferencesReset'));
    try {
      window.localStorage.removeItem(ADMIN_DASHBOARD_STATE_KEY);
    } catch {
      // ignore storage remove failures
    }
  };

  const openTopbarHistorySearch = useCallback(
    ({
      search,
      mode = 'player',
      actionType = 'any',
    }: {
      search: string;
      mode?: 'actionId' | 'player' | 'reason';
      actionType?: 'any' | AdminHistoryActionType;
    }) => {
      emitMenuDebug({
        source: `cross-search:${mode}`,
        previous: {
          surface: activeMenuSurface,
          tab: activeTab,
          topSection: activeTopSection,
        },
        next: {
          surface: 'topbar',
          tab: activeTab,
          topSection: 'history',
        },
        visible,
        ts: Date.now(),
      });
      setHistorySearch(search.trim());
      setHistorySearchMode(mode);
      setHistoryActionType(actionType);
      setHistoryAdmin('any');
      setActiveTopSection('history');
      setActiveMenuSurface('topbar');
      setSystemSectionActive(false);
    },
    [
      activeMenuSurface,
      activeTab,
      activeTopSection,
      emitMenuDebug,
      visible,
    ],
  );

  const tabs: Array<{ key: Tab; label: string }> = [
    { key: 'overview', label: t('adminDashboard.tabOverview') },
    { key: 'players', label: t('adminDashboard.tabPlayers') },
    { key: 'console', label: t('adminDashboard.tabConsole') },
    { key: 'logs', label: t('adminDashboard.tabLogs') },
    { key: 'metrics', label: t('adminDashboard.tabMetrics') },
    {
      key: 'resources',
      label: t('adminDashboard.sideTx_resources', {
        defaultValue: 'Resources',
      }),
    },
    { key: 'cfg', label: t('adminDashboard.tabCfg') },
    { key: 'respawn', label: t('adminDashboard.tabRespawn') },
    { key: 'events', label: t('adminDashboard.tabEvents') },
  ];

  const topNavItems: Array<{ key: TopSection; label: string }> = [
    { key: 'players', label: t('adminDashboard.topTx_players') },
    { key: 'history', label: t('adminDashboard.topTx_history') },
    { key: 'playerDrops', label: t('adminDashboard.topTx_playerDrops') },
    { key: 'whitelist', label: t('adminDashboard.topTx_whitelist') },
    { key: 'admins', label: t('adminDashboard.topTx_admins') },
    { key: 'settings', label: t('adminDashboard.topTx_settings') },
  ];

  const capabilityRows: Array<{ key: keyof AdminCapabilities; label: string }> =
    [
      { key: 'canKick', label: t('adminDashboard.capability_kick') },
      { key: 'canBan', label: t('adminDashboard.capability_ban') },
      { key: 'canUnban', label: t('adminDashboard.capability_unban') },
      { key: 'canConsole', label: t('adminDashboard.capability_console') },
      { key: 'canViewLogs', label: t('adminDashboard.capability_logs') },
      { key: 'canMessage', label: t('adminDashboard.capability_message') },
      { key: 'canMute', label: t('adminDashboard.capability_mute') },
      { key: 'canUnmute', label: t('adminDashboard.capability_unmute') },
      {
        key: 'canManageRespawn',
        label: t('adminDashboard.capability_manageRespawn'),
      },
    ];

  const sideNavItems: Array<{ key: Tab; label: string }> = [
    { key: 'overview', label: t('adminDashboard.sideTx_dashboard') },
    {
      key: 'players',
      label: t('adminDashboard.sideTx_players', { defaultValue: 'Players' }),
    },
    { key: 'console', label: t('adminDashboard.sideTx_liveConsole') },
    {
      key: 'resources',
      label: t('adminDashboard.sideTx_resources', {
        defaultValue: 'Resources',
      }),
    },
    { key: 'metrics', label: t('adminDashboard.tabMetrics') },
    { key: 'logs', label: t('adminDashboard.sideTx_serverLog') },
    { key: 'cfg', label: t('adminDashboard.sideTx_cfgEditor') },
    { key: 'respawn', label: t('adminDashboard.sideTx_respawnCenter') },
    { key: 'events', label: t('adminDashboard.sideTx_revivalJournal') },
  ];

  const currentSidebarLabel =
    tabs.find((tab) => tab.key === activeTab)?.label ??
    t('adminDashboard.title');
  const currentTopSectionLabel =
    topNavItems.find((item) => item.key === activeTopSection)?.label ??
    t('adminDashboard.subtitle');
  const currentMainTitle =
    activeMenuSurface === 'topbar'
      ? currentTopSectionLabel
      : currentSidebarLabel;
  const currentMainSubtitle =
    activeMenuSurface === 'topbar'
      ? t('adminDashboard.topbarSectionSubtitle', {
          defaultValue: 'Topbar Section',
        })
      : t('adminDashboard.sidebarSectionSubtitle', {
          defaultValue: 'Sidebar Section',
        });
  const playerRatio =
    status && status.maxPlayers > 0 ? status.online / status.maxPlayers : 0;
  const summaryCards = [
    {
      label: t('adminDashboard.players'),
      value: status ? `${status.online}/${status.maxPlayers}` : '0/0',
      tone: playerRatio >= 0.9 ? 'warn' : 'accent',
    },
    {
      label: t('adminDashboard.bannedUsers'),
      value: String(activeBannedUsers.length),
      tone: activeBannedUsers.length > 0 ? 'warn' : 'neutral',
    },
    {
      label: t('adminDashboard.mutedUsers'),
      value: String(activeMutedUsers.length),
      tone: activeMutedUsers.length > 0 ? 'warn' : 'neutral',
    },
    {
      label: t('adminDashboard.tabRespawn'),
      value: String(downedPlayers.length),
      tone: downedPlayers.length > 0 ? 'warn' : 'neutral',
    },
  ];
  const visibleRailPlayers = filteredPlayers
    .filter((player) => isPlayerOnline(player))
    .slice(0, 12);
  const selectedInventoryEntries = useMemo(() => {
    const rawEntries = playerInventory?.inventory?.entries;
    return Array.isArray(rawEntries) ? rawEntries : [];
  }, [playerInventory]);
  const normalizedConsoleSearch = consoleSearch.trim().toLowerCase();
  const filteredServerConsoleEntries = useMemo(
    () =>
      normalizedConsoleSearch.length === 0
        ? serverConsoleEntries
        : serverConsoleEntries.filter((entry) =>
            `${entry.level ?? ''} ${entry.message}`
              .toLowerCase()
              .includes(normalizedConsoleSearch),
          ),
    [normalizedConsoleSearch, serverConsoleEntries],
  );
  const normalizedLogTextFilter = logTextFilter.trim().toLowerCase();
  const filteredLogEntries = useMemo(
    () =>
      normalizedLogTextFilter.length === 0
        ? logEntries
        : logEntries.filter((entry) =>
            `${entry.type} ${entry.level ?? ''} ${entry.message}`
              .toLowerCase()
              .includes(normalizedLogTextFilter),
          ),
    [logEntries, normalizedLogTextFilter],
  );
  const hasActiveLogFilters = Boolean(
    logTypeFilter ||
      logLevelFilter ||
      logSinceMinutes ||
      normalizedLogTextFilter,
  );
  const topbarSystemHealthState = useMemo(() => {
    if (!topbarSystemStatus) return 'unknown' as const;
    if (
      topbarSystemStatus.online >= topbarSystemStatus.maxPlayers &&
      topbarSystemStatus.maxPlayers > 0
    )
      return 'critical' as const;
    if (
      topbarSystemStatus.online >=
      Math.max(1, Math.floor(topbarSystemStatus.maxPlayers * 0.9))
    )
      return 'warn' as const;
    return 'ok' as const;
  }, [topbarSystemStatus]);
  const installedVersionLabel =
    updateStatus.installedVersion ||
    t('adminDashboard.updateUnknownInstalled', {
      defaultValue: 'unknown',
    });
  const latestVersionLabel =
    updateStatus.latestVersion ||
    t('adminDashboard.updateUnknownLatest', {
      defaultValue: 'unavailable',
    });
  const formattedReleasePublishedAt = useMemo(() => {
    if (!updateStatus.publishedAt) return '';
    const parsed = new Date(updateStatus.publishedAt);
    if (Number.isNaN(parsed.getTime())) return '';
    return parsed.toLocaleString();
  }, [updateStatus.publishedAt]);
  const runtimeReadiness = updateStatus.runtimeReadiness;
  const runtimeOverall = runtimeReadiness?.overall || null;
  const runtimeChecks = runtimeReadiness?.checks || [];
  const runtimeCheckedAt = useMemo(() => {
    if (!runtimeReadiness?.checkedAt) return '';
    const parsed = new Date(runtimeReadiness.checkedAt);
    if (Number.isNaN(parsed.getTime())) return '';
    return parsed.toLocaleString();
  }, [runtimeReadiness?.checkedAt]);

  if (!visible) return <></>;

  return (
    <div
      className="admin-overlay"
      role="dialog"
      aria-modal="true"
      aria-label={t('adminDashboard.title')}
    >
      <div className="admin-dashboard">
        <div className="admin-dashboard__topbar">
          <div className="admin-dashboard__brand">
            <div className="admin-dashboard__brand-mark">SK</div>
            <div className="admin-dashboard__brand-text">
              <h1 className="admin-dashboard__title">
                {t('adminDashboard.title')}
              </h1>
              <p className="admin-dashboard__subtitle">
                {status?.name || t('adminDashboard.subtitle')}
              </p>
            </div>
          </div>

          <div
            className="admin-dashboard__topnav"
            role="tablist"
            aria-label={t('adminDashboard.title')}
          >
            {topNavItems.map(({ key, label }) => (
              <button
                key={key}
                type="button"
                id={`admin-top-tab-${key}`}
                data-testid={`admin-top-tab-${key}`}
                role="tab"
                aria-selected={activeTopSection === key}
                aria-controls={`admin-top-panel-${key}`}
                className={`admin-dashboard__topnav-item${
                  activeTopSection === key
                    ? ' admin-dashboard__topnav-item--active'
                    : ''
                }`}
                onClick={() => {
                  emitMenuDebug({
                    source: `topbar-click:${key}`,
                    previous: {
                      surface: activeMenuSurface,
                      tab: activeTab,
                      topSection: activeTopSection,
                    },
                    next: {
                      surface: 'topbar',
                      tab: activeTab,
                      topSection: key,
                    },
                    visible,
                    ts: Date.now(),
                  });
                  setActiveTopSection(key);
                  setActiveMenuSurface('topbar');
                  setSystemSectionActive(false);
                }}
              >
                {label}
              </button>
            ))}

            {/* System Dropdown Menu */}
            <div className="admin-dashboard__topnav-dropdown">
              <button
                type="button"
                className={`admin-dashboard__topnav-item admin-dashboard__topnav-item--dropdown${
                  systemDropdownOpen
                    ? ' admin-dashboard__topnav-item--active'
                    : ''
                }`}
                onClick={() => setSystemDropdownOpen(!systemDropdownOpen)}
              >
                {t('adminDashboard.topTx_system', { defaultValue: 'System' })}
                <span className="admin-dashboard__dropdown-arrow">▾</span>
              </button>
              {systemDropdownOpen && (
                <div className="admin-dashboard__topnav-dropdown-menu">
                  <button
                    type="button"
                    className={`admin-dashboard__dropdown-item${
                      activeSystemSubsection === 'masterActions'
                        ? ' admin-dashboard__dropdown-item--active'
                        : ''
                    }`}
                    onClick={() => {
                      setActiveSystemSubsection('masterActions');
                      setSystemSectionActive(true);
                      setSystemDropdownOpen(false);
                    }}
                  >
                    {t('adminDashboard.system_masterActions', {
                      defaultValue: 'Master Actions',
                    })}
                  </button>
                  <button
                    type="button"
                    className={`admin-dashboard__dropdown-item${
                      activeSystemSubsection === 'diagnostics'
                        ? ' admin-dashboard__dropdown-item--active'
                        : ''
                    }`}
                    onClick={() => {
                      setActiveSystemSubsection('diagnostics');
                      setSystemSectionActive(true);
                      setSystemDropdownOpen(false);
                    }}
                  >
                    {t('adminDashboard.system_diagnostics', {
                      defaultValue: 'Diagnostics',
                    })}
                  </button>
                  <button
                    type="button"
                    className={`admin-dashboard__dropdown-item${
                      activeSystemSubsection === 'consoleLog'
                        ? ' admin-dashboard__dropdown-item--active'
                        : ''
                    }`}
                    onClick={() => {
                      setActiveSystemSubsection('consoleLog');
                      setSystemSectionActive(true);
                      setSystemDropdownOpen(false);
                    }}
                  >
                    {t('adminDashboard.system_consoleLog', {
                      defaultValue: 'Console Log',
                    })}
                  </button>
                  <button
                    type="button"
                    className={`admin-dashboard__dropdown-item${
                      activeSystemSubsection === 'actionLog'
                        ? ' admin-dashboard__dropdown-item--active'
                        : ''
                    }`}
                    onClick={() => {
                      setActiveSystemSubsection('actionLog');
                      setSystemSectionActive(true);
                      setSystemDropdownOpen(false);
                    }}
                  >
                    {t('adminDashboard.system_actionLog', {
                      defaultValue: 'Action Log',
                    })}
                  </button>
                </div>
              )}
            </div>
          </div>

          <div className="admin-dashboard__topbar-meta">
            <span
              className={`admin-dashboard__ws-pill admin-dashboard__ws-pill--${wsHealthTone}`}
              title={t('adminDashboard.wsHealthTitle', {
                defaultValue: 'WebSocket updates health',
              })}
            >
              <span
                className={`admin-dashboard__ws-pill-dot admin-dashboard__ws-pill-dot--${wsHealthTone}`}
                aria-hidden="true"
              />
              {wsHealthLabel}
            </span>
            <span
              className={`admin-dashboard__role admin-dashboard__role--${adminRole}`}
            >
              {adminUser || t(`adminDashboard.role_${adminRole}`)}
            </span>
            <FrameButton
              name="closeAdmin"
              text={t('adminDashboard.exit')}
              variant="DEFAULT"
              width={112}
              height={40}
              onClick={hide}
            />
          </div>
        </div>

        <div className="admin-dashboard__chrome">
          <aside className="admin-dashboard__sidebar">
            <div className="admin-dashboard__sidebar-card admin-dashboard__sidebar-card--brand">
              <div className="admin-dashboard__server-name">
                {status?.name || 'SkyMP'}
              </div>
              <div className="admin-dashboard__server-meta">
                {t('adminDashboard.role')}:{' '}
                {t(`adminDashboard.role_${adminRole}`)}
              </div>
            </div>

            <div className="admin-dashboard__sidebar-card">
              <div className="admin-dashboard__sidebar-section-title">
                {t('adminDashboard.sideSectionNavigation', {
                  defaultValue: 'Navigation',
                })}
              </div>
              <div className="admin-dashboard__nav-list">
                {sideNavItems.map((item) => (
                  <button
                    key={item.key}
                    type="button"
                    id={`admin-tab-${item.key}`}
                    data-testid={`admin-tab-${item.key}`}
                    className={`admin-dashboard__nav-item${
                      activeTab === item.key
                        ? ' admin-dashboard__nav-item--active'
                        : ''
                    }`}
                    onClick={() => {
                      emitMenuDebug({
                        source: `sidebar-click:${item.key}`,
                        previous: {
                          surface: activeMenuSurface,
                          tab: activeTab,
                          topSection: activeTopSection,
                        },
                        next: {
                          surface: 'sidebar',
                          tab: item.key,
                          topSection: activeTopSection,
                        },
                        visible,
                        ts: Date.now(),
                      });
                      setActiveTab(item.key);
                      setActiveMenuSurface('sidebar');
                    }}
                  >
                    {item.label}
                  </button>
                ))}
              </div>
            </div>

            <div className="admin-dashboard__sidebar-card">
              <div className="admin-dashboard__sidebar-section-title">
                {t('adminDashboard.sideSectionStatus', {
                  defaultValue: 'Server Status',
                })}
              </div>
              <div className="admin-dashboard__sidebar-status-row">
                <span className="admin-dashboard__sidebar-status-key">
                  {t('adminDashboard.online')}
                </span>
                <span
                  className={`admin-dashboard__sidebar-pill${
                    status ? ' admin-dashboard__sidebar-pill--online' : ''
                  }`}
                >
                  {status
                    ? t('adminDashboard.online')
                    : t('adminDashboard.loading')}
                </span>
              </div>
              <div className="admin-dashboard__sidebar-status-row">
                <span className="admin-dashboard__sidebar-status-key">
                  {t('adminDashboard.uptime')}
                </span>
                <span className="admin-dashboard__sidebar-pill">
                  {status ? formatAdminUptime(status.uptimeSec) : '-'}
                </span>
              </div>
              <div className="admin-dashboard__sidebar-status-row">
                <span className="admin-dashboard__sidebar-status-key">
                  {t('adminDashboard.port')}
                </span>
                <span className="admin-dashboard__sidebar-pill">
                  {status?.port ?? '-'}
                </span>
              </div>
              <div className="admin-dashboard__sidebar-status-row">
                <span className="admin-dashboard__sidebar-status-key">
                  {t('adminDashboard.players')}
                </span>
                <span className="admin-dashboard__sidebar-pill">
                  {status ? `${status.online}/${status.maxPlayers}` : '0'}
                </span>
              </div>
            </div>

            <div className="admin-dashboard__sidebar-card admin-dashboard__sidebar-card--actions">
              <div className="admin-dashboard__sidebar-section-title">
                {t('adminDashboard.sidebarActionsTitle')}
              </div>
              <div className="admin-dashboard__sidebar-actions-grid">
                <button
                  type="button"
                  className="admin-dashboard__sidebar-action"
                  disabled={
                    !capabilities.canConsole ||
                    !serverControlAvailable ||
                    sidebarActionSending !== null
                  }
                  onClick={() => {
                    void runServerControl('stop');
                  }}
                  title={
                    !serverControlAvailable
                      ? t('adminDashboard.serverControlUnavailable', {
                          defaultValue:
                            'Server control requires a configured supervisor.',
                        })
                      : undefined
                  }
                >
                  {sidebarActionSending === 'server-stop'
                    ? t('adminDashboard.loading')
                    : status
                    ? t('adminDashboard.stopServer')
                    : t('adminDashboard.startServer')}
                </button>
                <button
                  type="button"
                  className="admin-dashboard__sidebar-action"
                  disabled={
                    !capabilities.canConsole ||
                    !serverControlAvailable ||
                    sidebarActionSending !== null
                  }
                  onClick={() => {
                    void runServerControl('restart');
                  }}
                  title={
                    !serverControlAvailable
                      ? t('adminDashboard.serverControlUnavailable', {
                          defaultValue:
                            'Server control requires a configured supervisor.',
                        })
                      : undefined
                  }
                >
                  {sidebarActionSending === 'server-restart'
                    ? t('adminDashboard.loading')
                    : t('adminDashboard.restartServer')}
                </button>
                <button
                  type="button"
                  className="admin-dashboard__sidebar-action"
                  disabled={
                    !capabilities.canKick ||
                    players.length === 0 ||
                    sidebarActionSending !== null
                  }
                  onClick={() => {
                    void kickAllPlayers();
                  }}
                >
                  {sidebarActionSending === 'kick-all'
                    ? t('adminDashboard.loading')
                    : t('adminDashboard.kickAllPlayers')}
                </button>
                <button
                  type="button"
                  className="admin-dashboard__sidebar-action"
                  disabled={
                    !capabilities.canMessage || sidebarActionSending !== null
                  }
                  onClick={() => {
                    void sendAnnouncement();
                  }}
                >
                  {sidebarActionSending === 'announcement'
                    ? t('adminDashboard.loading')
                    : t('adminDashboard.sendAnnouncement')}
                </button>
                <button
                  type="button"
                  className="admin-dashboard__sidebar-action admin-dashboard__sidebar-action--secondary"
                  onClick={resetDashboardPreferences}
                >
                  {t('adminDashboard.resetView')}
                </button>
              </div>
              <div className="admin-dashboard__sidebar-action-hint">
                {t('adminDashboard.sidebarActionHint')}
              </div>
            </div>
          </aside>

          <main className="admin-dashboard__main">
            <div className="admin-dashboard__main-header">
                          {activeTab === 'players' && activeMenuSurface === 'sidebar' && (
                            <div className="admin-dashboard__players-table-wrap">
                              <table className="admin-dashboard__players-table">
                                <thead>
                                  <tr>
                                    <th>{t('adminDashboard.playerName', { defaultValue: 'Name' })}</th>
                                    <th>{t('adminDashboard.playerId', { defaultValue: 'User ID' })}</th>
                                    <th>{t('adminDashboard.playerActions', { defaultValue: 'Aktionen' })}</th>
                                  </tr>
                                </thead>
                                <tbody>
                                  {filteredPlayers.map((player) => (
                                    <tr key={`player-row-${player.userId}`}>
                                      <td>
                                        <button
                                          type="button"
                                          style={{ background: 'none', border: 'none', color: '#7ec8c8', cursor: 'pointer', padding: 0, textDecoration: 'underline' }}
                                          onClick={() => { setProfileModalUserId(player.userId); setProfileModalName(player.actorName || `userId=${player.userId}`); }}
                                        >
                                          {player.actorName || `userId=${player.userId}`}
                                        </button>
                                      </td>
                                      <td>{player.userId}</td>
                                      <td>
                                        {capabilities.canKick && (
                                          <button type="button" onClick={() => kickPlayer(player.userId)}>
                                            {t('adminDashboard.kick', { defaultValue: 'Kick' })}
                                          </button>
                                        )}
                                        {capabilities.canBan && (
                                          <button type="button" onClick={() => banPlayer(player.userId)}>
                                            {t('adminDashboard.ban', { defaultValue: 'Ban' })}
                                          </button>
                                        )}
                                        {capabilities.canMute && (
                                          <button type="button" onClick={() => mutePlayer(player.userId)}>
                                            {t('adminDashboard.mute', { defaultValue: 'Mute' })}
                                          </button>
                                        )}
                                        {capabilities.canMessage && (
                                          <button type="button" onClick={() => { setSendMsgTargetId(player.userId); setSendMsgTargetName(player.actorName || `userId=${player.userId}`); }}>
                                            {t('adminDashboard.message', { defaultValue: 'Nachricht' })}
                                          </button>
                                        )}
                                        {capabilities.canWarn && (
                                          <button
                                            type="button"
                                            data-testid={`admin-warn-btn-${player.userId}`}
                                            style={{ color: '#e6b800', borderColor: '#e6b800', marginLeft: 4 }}
                                            onClick={() => openWarnDialog(player.userId, player.actorName || `userId=${player.userId}`)}
                                          >
                                            {t('adminDashboard.warn', { defaultValue: 'Warnen' })}
                                          </button>
                                        )}
                                        {capabilities.canViewLogs && (
                                          <button
                                            type="button"
                                            style={{ marginLeft: 4 }}
                                            data-testid={`admin-history-btn-${player.userId}`}
                                            onClick={() =>
                                              openTopbarHistorySearch({
                                                search: String(player.userId),
                                                mode: 'player',
                                              })
                                            }
                                          >
                                            {t('adminDashboard.topTx_history', {
                                              defaultValue: 'History',
                                            })}
                                          </button>
                                        )}
                                      </td>
                                    </tr>
                                  ))}
                                  {filteredPlayers.length === 0 && (
                                    <tr>
                                      <td colSpan={3}>
                                        {t('adminDashboard.noPlayersStored', {
                                          defaultValue: 'No players found yet',
                                        })}
                                      </td>
                                    </tr>
                                  )}
                                </tbody>
                              </table>
                            </div>
                          )}

                          {/* Player Profile Modal */}
                          {profileModalUserId !== null && (
                            <PlayerProfileModal
                              userId={profileModalUserId}
                              playerName={profileModalName}
                              onClose={() => setProfileModalUserId(null)}
                              canBan={capabilities.canBan}
                              canWarn={capabilities.canWarn}
                              onKick={capabilities.canKick ? (id) => kickPlayer(id) : undefined}
                              onBan={capabilities.canBan ? (id) => banPlayer(id) : undefined}
                              onWarn={capabilities.canWarn ? (id, name) => openWarnDialog(id, name) : undefined}
                            />
                          )}

                          {/* Warn Dialog */}
                          {warnDialogOpen && (
                            <div className="admin-dashboard__warn-modal-backdrop" onClick={closeWarnDialog}>
                              <div className="admin-dashboard__warn-modal" onClick={e => e.stopPropagation()}>
                                <div className="admin-dashboard__warn-modal-head">
                                  <h3>{t('adminDashboard.warnTitle', { defaultValue: 'Warnen' })}</h3>
                                  <button type="button" onClick={closeWarnDialog} disabled={warnSending}>×</button>
                                </div>
                                <div data-testid="admin-warn-form" className="admin-dashboard__warn-modal-body">
                                  <div>{t('adminDashboard.warnTarget', { defaultValue: 'Spieler:' })} <b>{warnTargetName}</b></div>
                                  <label>
                                    {t('adminDashboard.warnMessage', { defaultValue: 'Nachricht' })}
                                    <input
                                      type="text"
                                      data-testid="admin-warn-message-input"
                                      value={warnMessage}
                                      onChange={e => setWarnMessage(e.target.value)}
                                      placeholder={t('adminDashboard.warnMessagePlaceholder', { defaultValue: 'Bitte Nachricht eingeben...' })}
                                      disabled={warnSending}
                                      autoFocus
                                    />
                                  </label>
                                  <label>
                                    {t('adminDashboard.warnReason', { defaultValue: 'Grund (optional)' })}
                                    <input
                                      type="text"
                                      data-testid="admin-warn-reason-input"
                                      aria-invalid={warnReasonInvalid}
                                      value={warnReason}
                                      onChange={e => setWarnReason(e.target.value)}
                                      placeholder={t('adminDashboard.warnReasonPlaceholder', { defaultValue: 'Optionaler Grund...' })}
                                      disabled={warnSending}
                                      maxLength={MAX_MODERATION_REASON_LENGTH + 1}
                                    />
                                  </label>
                                </div>
                                <div className="admin-dashboard__warn-modal-foot">
                                  <button type="button" data-testid="admin-warn-cancel-btn" onClick={closeWarnDialog} disabled={warnSending}>
                                    {t('adminDashboard.cancel', { defaultValue: 'Abbrechen' })}
                                  </button>
                                  <button
                                    type="button"
                                    data-testid="admin-warn-submit-btn"
                                    className="admin-dashboard__warn-submit"
                                    onClick={sendWarn}
                                    disabled={warnSending || !warnMessage.trim()}
                                  >
                                    {warnSending
                                      ? t('adminDashboard.sending', { defaultValue: 'Sende...' })
                                      : t('adminDashboard.warnSend', { defaultValue: 'Warnen' })}
                                  </button>
                                </div>
                              </div>
                            </div>
                          )}
              <div>
                <h2 className="admin-dashboard__main-title">
                  {currentMainTitle}
                </h2>
                <p className="admin-dashboard__main-subtitle">
                  {currentMainSubtitle}
                </p>
              </div>
              <div className="admin-dashboard__main-meta">
                {lastUpdated && (
                  <span className="admin-dashboard__updated">
                    {t('adminDashboard.updated')}: {lastUpdated}
                  </span>
                )}
              </div>
            </div>

            <section
              className={`admin-dashboard__update-banner${
                updateStatus.updateAvailable
                  ? ' admin-dashboard__update-banner--available'
                  : ''
              }`}
              aria-label={t('adminDashboard.updateBannerTitle', {
                defaultValue: 'Server update status',
              })}
            >
              <div className="admin-dashboard__update-banner-copy">
                <strong className="admin-dashboard__update-banner-title">
                  {updateStatus.updateAvailable
                    ? t('adminDashboard.updateAvailableTitle', {
                        defaultValue: 'New server version available',
                      })
                    : t('adminDashboard.updateCurrentTitle', {
                        defaultValue: 'Server version status',
                      })}
                </strong>
                <span className="admin-dashboard__update-banner-text">
                  {t('adminDashboard.updateInstalledLabel', {
                    defaultValue: 'Installed',
                  })}
                  : {installedVersionLabel} · {t('adminDashboard.updateLatestLabel', {
                    defaultValue: 'Latest',
                  })}
                  : {latestVersionLabel}
                  {formattedReleasePublishedAt
                    ? ` · ${t('adminDashboard.updatePublishedLabel', {
                        defaultValue: 'Published',
                      })}: ${formattedReleasePublishedAt}`
                    : ''}
                  {runtimeOverall
                    ? ` · ${t('adminDashboard.runtimeReadinessOverall', {
                        defaultValue: 'Runtime',
                      })}: ${t(`adminDashboard.runtimeStatus_${runtimeOverall}`, {
                        defaultValue: runtimeOverall,
                      })}`
                    : ''}
                </span>
              </div>
              <div className="admin-dashboard__update-banner-actions">
                <button
                  type="button"
                  className="admin-dashboard__update-btn"
                  onClick={() => setUpdateModalOpen(true)}
                >
                  {t('adminDashboard.updateViewChangelog', {
                    defaultValue: 'View changelog',
                  })}
                </button>
                {updateStatus.releaseUrl && (
                  <a
                    className="admin-dashboard__update-btn admin-dashboard__update-btn--link"
                    href={updateStatus.releaseUrl}
                    target="_blank"
                    rel="noreferrer"
                  >
                    {t('adminDashboard.updateOpenRelease', {
                      defaultValue: 'Open release',
                    })}
                  </a>
                )}
                <button
                  type="button"
                  className="admin-dashboard__update-btn admin-dashboard__update-btn--secondary"
                  onClick={() => {
                    void fetchUpdateStatus();
                  }}
                  disabled={updateStatusLoading}
                >
                  {updateStatusLoading
                    ? t('adminDashboard.loading')
                    : t('adminDashboard.updateRefresh', {
                        defaultValue: 'Refresh',
                      })}
                </button>
              </div>
            </section>

            {updateModalOpen && (
              <div
                className="admin-dashboard__update-modal-backdrop"
                onClick={() => setUpdateModalOpen(false)}
              >
                <div
                  className="admin-dashboard__update-modal"
                  role="dialog"
                  aria-modal="true"
                  aria-label={t('adminDashboard.updateModalTitle', {
                    defaultValue: 'Server update changelog',
                  })}
                  onClick={(event) => event.stopPropagation()}
                >
                  <div className="admin-dashboard__update-modal-head">
                    <div>
                      <h3>
                        {t('adminDashboard.updateModalTitle', {
                          defaultValue: 'Server update changelog',
                        })}
                      </h3>
                      <p>
                        {t('adminDashboard.updateInstalledLabel', {
                          defaultValue: 'Installed',
                        })}
                        : {installedVersionLabel} · {t(
                          'adminDashboard.updateLatestLabel',
                          {
                            defaultValue: 'Latest',
                          },
                        )}
                        : {latestVersionLabel}
                      </p>
                    </div>
                    <button
                      type="button"
                      onClick={() => setUpdateModalOpen(false)}
                    >
                      ×
                    </button>
                  </div>
                  <div className="admin-dashboard__update-modal-body">
                    {updateStatus.error && (
                      <p className="admin-dashboard__update-modal-error">
                        {t('adminDashboard.updateFetchError', {
                          defaultValue:
                            'Latest release information is currently unavailable.',
                        })}
                      </p>
                    )}
                    <section className="admin-dashboard__runtime-readiness">
                      <h4 className="admin-dashboard__runtime-readiness-title">
                        {t('adminDashboard.runtimeReadinessTitle', {
                          defaultValue: 'Runtime readiness',
                        })}
                      </h4>
                      {runtimeCheckedAt && (
                        <p className="admin-dashboard__runtime-readiness-meta">
                          {t('adminDashboard.runtimeReadinessCheckedAt', {
                            defaultValue: 'Checked at',
                          })}
                          : {runtimeCheckedAt}
                        </p>
                      )}
                      {runtimeChecks.length === 0 ? (
                        <p className="admin-dashboard__runtime-readiness-empty">
                          {t('adminDashboard.runtimeReadinessNoData', {
                            defaultValue: 'No runtime readiness data available.',
                          })}
                        </p>
                      ) : (
                        <ul className="admin-dashboard__runtime-readiness-list">
                          {runtimeChecks.map((check) => (
                            <li
                              key={`${check.id}:${check.code}`}
                              className={`admin-dashboard__runtime-readiness-item admin-dashboard__runtime-readiness-item--${check.level}`}
                            >
                              <span className="admin-dashboard__runtime-readiness-item-status">
                                {t(`adminDashboard.runtimeStatus_${check.level}`, {
                                  defaultValue: check.level,
                                })}
                              </span>
                              <span className="admin-dashboard__runtime-readiness-item-text">
                                {t(`adminDashboard.runtimeCheckCode_${check.code}`, {
                                  defaultValue: check.message,
                                })}
                              </span>
                            </li>
                          ))}
                        </ul>
                      )}
                    </section>
                    <pre className="admin-dashboard__update-modal-changelog">
                      {updateStatus.changelog ||
                        t('adminDashboard.updateNoChangelog', {
                          defaultValue: 'No changelog text available for the latest release.',
                        })}
                    </pre>
                  </div>
                  <div className="admin-dashboard__update-modal-foot">
                    {updateStatus.releaseUrl && (
                      <a
                        className="admin-dashboard__update-btn admin-dashboard__update-btn--link"
                        href={updateStatus.releaseUrl}
                        target="_blank"
                        rel="noreferrer"
                      >
                        {t('adminDashboard.updateOpenRelease', {
                          defaultValue: 'Open release',
                        })}
                      </a>
                    )}
                    <button
                      type="button"
                      className="admin-dashboard__update-btn admin-dashboard__update-btn--secondary"
                      onClick={() => setUpdateModalOpen(false)}
                    >
                      {t('adminDashboard.exit')}
                    </button>
                  </div>
                </div>
              </div>
            )}

            {/* System Dropdown Subsections */}
            {systemSectionActive && (
              <section
                className="admin-dashboard__system-hub"
                id="admin-top-panel-system"
                role="tabpanel"
                aria-labelledby="admin-top-tab-system"
                aria-label={t('adminDashboard.systemHubTitle', {
                  defaultValue: 'System Hub',
                })}
              >
                <div className="admin-dashboard__system-hub-header">
                  <h3 className="admin-dashboard__system-hub-title">
                    {activeSystemSubsection === 'masterActions' &&
                      t('adminDashboard.system_masterActions', {
                        defaultValue: 'Master Actions',
                      })}
                    {activeSystemSubsection === 'diagnostics' &&
                      t('adminDashboard.system_diagnostics', {
                        defaultValue: 'Diagnostics',
                      })}
                    {activeSystemSubsection === 'consoleLog' &&
                      t('adminDashboard.system_consoleLog', {
                        defaultValue: 'Console Log',
                      })}
                    {activeSystemSubsection === 'actionLog' &&
                      t('adminDashboard.system_actionLog', {
                        defaultValue: 'Action Log',
                      })}
                  </h3>
                  <span
                    className={`admin-dashboard__system-health-pill admin-dashboard__system-health-pill--${topbarSystemHealthState}`}
                  >
                    {t(
                      `adminDashboard.systemHealth_${topbarSystemHealthState}`,
                      { defaultValue: topbarSystemHealthState },
                    )}
                  </span>
                </div>

                {/* Master Actions Subsection */}
                {activeSystemSubsection === 'masterActions' && (
                  <div className="admin-dashboard__system-hub-grid">
                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.systemCard_process', {
                          defaultValue: 'Process Control',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.systemCard_processDesc', {
                          defaultValue:
                            'Restart, stop and supervise the running server process.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-actions">
                        <button
                          type="button"
                          className="admin-dashboard__sidebar-action"
                          disabled={
                            !capabilities.canConsole ||
                            !serverControlAvailable ||
                            sidebarActionSending !== null
                          }
                          onClick={() => {
                            void runServerControl('restart');
                          }}
                          title={
                            !serverControlAvailable
                              ? t('adminDashboard.serverControlUnavailable', {
                                  defaultValue:
                                    'Server control requires a configured supervisor.',
                                })
                              : undefined
                          }
                        >
                          {t('adminDashboard.restartServer')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__sidebar-action"
                          disabled={
                            !capabilities.canConsole ||
                            !serverControlAvailable ||
                            sidebarActionSending !== null
                          }
                          onClick={() => {
                            void runServerControl('stop');
                          }}
                          title={
                            !serverControlAvailable
                              ? t('adminDashboard.serverControlUnavailable', {
                                  defaultValue:
                                    'Server control requires a configured supervisor.',
                                })
                              : undefined
                          }
                        >
                          {t('adminDashboard.stopServer')}
                        </button>
                      </div>
                    </article>
                  </div>
                )}

                {/* Diagnostics Subsection */}
                {activeSystemSubsection === 'diagnostics' && (
                  <div className="admin-dashboard__system-hub-grid">
                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.systemCard_observability', {
                          defaultValue: 'Observability',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.systemCard_observabilityDesc', {
                          defaultValue:
                            'Inspect runtime logs, metrics and live console output.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-meta">
                        {t('adminDashboard.online')}:{' '}
                        {topbarSystemStatus
                          ? `${topbarSystemStatus.online}/${topbarSystemStatus.maxPlayers}`
                          : '-'}
                      </div>
                      <div className="admin-dashboard__system-card-meta">
                        {t('adminDashboard.uptime')}:{' '}
                        {topbarSystemStatus
                          ? formatAdminUptime(topbarSystemStatus.uptimeSec)
                          : '-'}
                      </div>
                    </article>
                  </div>
                )}

                {/* Console Log Subsection */}
                {activeSystemSubsection === 'consoleLog' && (
                  <div className="admin-dashboard__system-hub-grid">
                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.system_consoleLog', {
                          defaultValue: 'Console Log',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.system_consoleLogDesc', {
                          defaultValue:
                            'Real-time server console output and debug logs.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-meta">
                        {serverConsoleEntries.length > 0
                          ? `${serverConsoleEntries.length} entries`
                          : 'No logs available'}
                      </div>
                    </article>
                  </div>
                )}

                {/* Action Log Subsection */}
                {activeSystemSubsection === 'actionLog' && (
                  <div className="admin-dashboard__system-hub-grid">
                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.system_actionLog', {
                          defaultValue: 'Action Log',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.system_actionLogDesc', {
                          defaultValue:
                            'Audit trail of all administrative actions and changes.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-meta">
                        {serverConsoleEntries.length > 0
                          ? `${serverConsoleEntries.length} actions logged`
                          : 'No actions logged'}
                      </div>
                    </article>
                  </div>
                )}
              </section>
            )}

            {activeMenuSurface === 'topbar' &&
              activeTopSection === 'system' && (
                <section
                  className="admin-dashboard__system-hub"
                  id="admin-top-panel-system"
                  role="tabpanel"
                  aria-labelledby="admin-top-tab-system"
                  aria-label={t('adminDashboard.systemHubTitle', {
                    defaultValue: 'System Hub',
                  })}
                >
                  <div className="admin-dashboard__system-hub-header">
                    <h3 className="admin-dashboard__system-hub-title">
                      {t('adminDashboard.systemHubTitle', {
                        defaultValue: 'System Hub',
                      })}
                    </h3>
                    <span
                      className={`admin-dashboard__system-health-pill admin-dashboard__system-health-pill--${topbarSystemHealthState}`}
                    >
                      {t(
                        `adminDashboard.systemHealth_${topbarSystemHealthState}`,
                        { defaultValue: topbarSystemHealthState },
                      )}
                    </span>
                  </div>

                  <div className="admin-dashboard__system-hub-grid">
                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.systemCard_process', {
                          defaultValue: 'Process Control',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.systemCard_processDesc', {
                          defaultValue:
                            'Restart, stop and supervise the running server process.',
                        })}
                      </p>
                      {!serverControlAvailable && (
                        <p>
                          {t('adminDashboard.serverControlUnavailable', {
                            defaultValue:
                              'Server control requires a configured supervisor.',
                          })}
                        </p>
                      )}
                      <div className="admin-dashboard__system-card-actions">
                        <button
                          type="button"
                          className="admin-dashboard__sidebar-action"
                          disabled={
                            !capabilities.canConsole ||
                            !serverControlAvailable ||
                            sidebarActionSending !== null
                          }
                          onClick={() => {
                            void runServerControl('restart');
                          }}
                          title={
                            !serverControlAvailable
                              ? t('adminDashboard.serverControlUnavailable', {
                                  defaultValue:
                                    'Server control requires a configured supervisor.',
                                })
                              : undefined
                          }
                        >
                          {t('adminDashboard.restartServer')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__sidebar-action"
                          disabled={
                            !capabilities.canConsole ||
                            !serverControlAvailable ||
                            sidebarActionSending !== null
                          }
                          onClick={() => {
                            void runServerControl('stop');
                          }}
                          title={
                            !serverControlAvailable
                              ? t('adminDashboard.serverControlUnavailable', {
                                  defaultValue:
                                    'Server control requires a configured supervisor.',
                                })
                              : undefined
                          }
                        >
                          {t('adminDashboard.stopServer')}
                        </button>
                      </div>
                    </article>

                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.systemCard_observability', {
                          defaultValue: 'Observability',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.systemCard_observabilityDesc', {
                          defaultValue:
                            'Inspect runtime logs, metrics and live console output.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-meta">
                        {t('adminDashboard.online')}:{' '}
                        {topbarSystemStatus
                          ? `${topbarSystemStatus.online}/${topbarSystemStatus.maxPlayers}`
                          : '-'}
                      </div>
                      <div className="admin-dashboard__system-card-meta">
                        {t('adminDashboard.uptime')}:{' '}
                        {topbarSystemStatus
                          ? formatAdminUptime(topbarSystemStatus.uptimeSec)
                          : '-'}
                      </div>
                    </article>

                    <article className="admin-dashboard__system-card">
                      <h4>
                        {t('adminDashboard.systemCard_configuration', {
                          defaultValue: 'Configuration',
                        })}
                      </h4>
                      <p>
                        {t('adminDashboard.systemCard_configurationDesc', {
                          defaultValue:
                            'Open the CFG editor and apply controlled server setting changes.',
                        })}
                      </p>
                      <div className="admin-dashboard__system-card-meta">
                        {t('adminDashboard.systemCard_configurationMeta', {
                          defaultValue:
                            'Configuration topics run in the dedicated Settings topbar page.',
                        })}
                      </div>
                    </article>
                  </div>
                </section>
              )}

            {activeMenuSurface === 'topbar' &&
              activeTopSection !== 'system' && (
                <section
                  className="admin-dashboard__system-hub"
                  id={`admin-top-panel-${activeTopSection}`}
                  role="tabpanel"
                  aria-labelledby={`admin-top-tab-${activeTopSection}`}
                >
                  <div className="admin-dashboard__system-hub-header">
                    <h3 className="admin-dashboard__system-hub-title">
                      {currentTopSectionLabel}
                    </h3>
                    <span className="admin-dashboard__system-health-pill admin-dashboard__system-health-pill--ok">
                      {t('adminDashboard.topbarPageLive', {
                        defaultValue: 'Live',
                      })}
                    </span>
                  </div>

                  <div className="admin-dashboard__system-hub-grid">
                    {activeTopSection === 'players' && (
                      <section
                        className="admin-dashboard__tx-players"
                        aria-label={t('adminDashboard.topTx_players')}
                      >
                        <div className="admin-dashboard__tx-players-cards">
                          <article className="admin-dashboard__tx-players-card">
                            <div className="admin-dashboard__tx-players-card-title">
                              {t('adminDashboard.topPlayers_total', {
                                defaultValue: 'Total Players',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-players-card-value">
                              {players.length}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-players-card">
                            <div className="admin-dashboard__tx-players-card-title">
                              {t('adminDashboard.topPlayers_last24h', {
                                defaultValue: 'Players Last 24h',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-players-card-value">
                              {filteredTopbarPlayers.length}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-players-card">
                            <div className="admin-dashboard__tx-players-card-title">
                              {t('adminDashboard.topPlayers_new24h', {
                                defaultValue: 'New Players Last 24h',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-players-card-value">
                              +{topbarNewPlayers24h}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-players-card">
                            <div className="admin-dashboard__tx-players-card-title">
                              {t('adminDashboard.topPlayers_new7d', {
                                defaultValue: 'New Players Last 7d',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-players-card-value">
                              +0
                            </div>
                          </article>
                        </div>

                        <div className="admin-dashboard__tx-players-toolbar">
                          <div className="admin-dashboard__tx-players-toolbar-top">
                            <input
                              className="admin-dashboard__tx-players-search"
                              type="text"
                              placeholder={t(
                                'adminDashboard.topPlayersSearchPlaceholder',
                                { defaultValue: 'Enter a player name' },
                              )}
                              aria-label={t(
                                'adminDashboard.topPlayersSearchPlaceholder',
                                { defaultValue: 'Enter a player name' },
                              )}
                              value={topbarPlayersSearch}
                              onChange={(e) =>
                                setTopbarPlayersSearch(e.target.value)
                              }
                            />
                            <select
                              className="admin-dashboard__tx-players-select"
                              value={topbarPlayersSearchMode}
                              onChange={(e) =>
                                setTopbarPlayersSearchMode(
                                  e.target.value as 'name' | 'id',
                                )
                              }
                              aria-label={t(
                                'adminDashboard.topPlayersSearchBy',
                                { defaultValue: 'Search by' },
                              )}
                            >
                              <option value="name">
                                {t('adminDashboard.topPlayersSearchByName', {
                                  defaultValue: 'Search by Name',
                                })}
                              </option>
                              <option value="id">
                                {t('adminDashboard.topPlayersSearchById', {
                                  defaultValue: 'Search by ID',
                                })}
                              </option>
                            </select>
                            <select
                              className="admin-dashboard__tx-players-select"
                              value={topbarPlayersFilter}
                              onChange={(e) =>
                                setTopbarPlayersFilter(
                                  e.target.value as 'none' | 'online' | 'offline',
                                )
                              }
                              aria-label={t('adminDashboard.topPlayersFilter', {
                                defaultValue: 'Filter',
                              })}
                            >
                              <option value="none">
                                {t('adminDashboard.topPlayersFilterNone', {
                                  defaultValue: 'No filters',
                                })}
                              </option>
                              <option value="online">
                                {t('adminDashboard.topPlayersFilterOnline', {
                                  defaultValue: 'Only online',
                                })}
                              </option>
                              <option value="offline">
                                {t('adminDashboard.topPlayersFilterOffline', {
                                  defaultValue: 'Only offline',
                                })}
                              </option>
                            </select>
                            <button
                              type="button"
                              className="admin-dashboard__tx-players-more"
                            >
                              {t('adminDashboard.topPlayersMore', {
                                defaultValue: 'More',
                              })}
                            </button>
                          </div>
                          <p className="admin-dashboard__tx-players-help">
                            {t('adminDashboard.topPlayersHelp', {
                              defaultValue:
                                'Search players by their last display name.',
                            })}
                          </p>
                        </div>

                        <div className="admin-dashboard__tx-players-table-wrap">
                          <table className="admin-dashboard__tx-players-table">
                            <thead>
                              <tr>
                                <th>
                                  {t('adminDashboard.topPlayersDisplayName', {
                                    defaultValue: 'Display Name',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topPlayersPlayTime', {
                                    defaultValue: 'Play Time',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topPlayersFirstJoined', {
                                    defaultValue: 'First Joined',
                                  })}
                                </th>
                                <th>
                                  {t(
                                    'adminDashboard.topPlayersLastConnection',
                                    { defaultValue: 'Last Connection' },
                                  )}
                                </th>
                              </tr>
                            </thead>
                            <tbody>
                              {filteredTopbarPlayers.map((player) => (
                                <tr key={`topbar-player-row-${player.userId}`}>
                                  <td>
                                    {player.actorName ||
                                      `userId=${player.userId}`}
                                    {!isPlayerOnline(player) && (
                                      <span className="admin-dashboard__top-player-offline-badge">
                                        {t('adminDashboard.topPlayersOffline', {
                                          defaultValue: 'offline',
                                        })}
                                      </span>
                                    )}
                                  </td>
                                  <td>
                                    {formatAdminUptime(
                                      Math.max(
                                        0,
                                        Math.floor(
                                          Number(player.playTimeSec || 0),
                                        ),
                                      ),
                                    )}
                                  </td>
                                  <td>
                                    {Number.isFinite(player.firstJoinedAt)
                                      ? formatAdminTime(
                                          Number(player.firstJoinedAt),
                                        )
                                      : '-'}
                                  </td>
                                  <td>
                                    {Number.isFinite(player.lastConnectionAt)
                                      ? formatAdminTime(
                                          Number(player.lastConnectionAt),
                                        )
                                      : '-'}
                                  </td>
                                </tr>
                              ))}
                              {filteredTopbarPlayers.length === 0 && (
                                <tr>
                                  <td colSpan={4}>
                                    {t('adminDashboard.noPlayersStored', {
                                      defaultValue: 'No players found yet',
                                    })}
                                  </td>
                                </tr>
                              )}
                            </tbody>
                          </table>
                        </div>
                      </section>
                    )}

                    {activeTopSection === 'history' && (
                      <section
                        className="admin-dashboard__tx-history"
                        aria-label={t('adminDashboard.topTx_history', {
                          defaultValue: 'History',
                        })}
                      >
                        <div className="admin-dashboard__tx-history-cards">
                          <article className="admin-dashboard__tx-history-card">
                            <div className="admin-dashboard__tx-history-card-icon">
                              &#9651;
                            </div>
                            <div className="admin-dashboard__tx-history-card-title">
                              {t('adminDashboard.topHistory_totalWarns', {
                                defaultValue: 'Total Warns',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-history-card-value">
                              {topbarHistoryData.totalWarns}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-history-card">
                            <div className="admin-dashboard__tx-history-card-icon">
                              &#9651;
                            </div>
                            <div className="admin-dashboard__tx-history-card-title">
                              {t('adminDashboard.topHistory_newWarns7d', {
                                defaultValue: 'New Warns Last 7d',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-history-card-value">
                              +{topbarHistoryData.newWarns7d}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-history-card">
                            <div className="admin-dashboard__tx-history-card-icon">
                              &#128296;
                            </div>
                            <div className="admin-dashboard__tx-history-card-title">
                              {t('adminDashboard.topHistory_totalBans', {
                                defaultValue: 'Total Bans',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-history-card-value">
                              {topbarHistoryData.totalBans}
                            </div>
                          </article>
                          <article className="admin-dashboard__tx-history-card">
                            <div className="admin-dashboard__tx-history-card-icon">
                              &#128296;
                            </div>
                            <div className="admin-dashboard__tx-history-card-title">
                              {t('adminDashboard.topHistory_newBans7d', {
                                defaultValue: 'New Bans Last 7d',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-history-card-value">
                              +{topbarHistoryData.newBans7d}
                            </div>
                          </article>
                        </div>

                        <div className="admin-dashboard__tx-history-toolbar">
                          <div className="admin-dashboard__tx-history-toolbar-top">
                            <input
                              className="admin-dashboard__tx-history-search"
                              type="text"
                              placeholder={
                                historySearchMode === 'actionId'
                                  ? 'XXXX-XXXX'
                                  : historySearchMode === 'player'
                                  ? t(
                                      'adminDashboard.topHistory_searchPlayer',
                                      { defaultValue: 'Player name or ID' },
                                    )
                                  : t(
                                      'adminDashboard.topHistory_searchReason',
                                      { defaultValue: 'Reason text' },
                                    )
                              }
                              value={historySearch}
                              onChange={(e) => setHistorySearch(e.target.value)}
                              aria-label={t(
                                'adminDashboard.topHistory_search',
                                { defaultValue: 'Search' },
                              )}
                            />
                            <select
                              className="admin-dashboard__tx-history-select"
                              value={historySearchMode}
                              onChange={(e) =>
                                setHistorySearchMode(
                                  e.target.value as
                                    | 'actionId'
                                    | 'player'
                                    | 'reason',
                                )
                              }
                              aria-label={t(
                                'adminDashboard.topHistory_searchBy',
                                { defaultValue: 'Search by' },
                              )}
                            >
                              <option value="actionId">
                                {t('adminDashboard.topHistory_byActionId', {
                                  defaultValue: 'Search by Action ID',
                                })}
                              </option>
                              <option value="player">
                                {t('adminDashboard.topHistory_byPlayer', {
                                  defaultValue: 'Search by Player',
                                })}
                              </option>
                              <option value="reason">
                                {t('adminDashboard.topHistory_byReason', {
                                  defaultValue: 'Search by Reason',
                                })}
                              </option>
                            </select>
                            <select
                              className="admin-dashboard__tx-history-select"
                              value={historyActionType}
                              onChange={(e) =>
                                setHistoryActionType(
                                  e.target.value as
                                    | 'any'
                                    | AdminHistoryActionType,
                                )
                              }
                              aria-label={t(
                                'adminDashboard.topHistory_filterType',
                                { defaultValue: 'Filter by type' },
                              )}
                            >
                              <option value="any">
                                {t('adminDashboard.topHistory_anyType', {
                                  defaultValue: 'Any type',
                                })}
                              </option>
                              <option value="warn">
                                {t('adminDashboard.topHistory_typeWarn', {
                                  defaultValue: 'Warn',
                                })}
                              </option>
                              <option value="ban">
                                {t('adminDashboard.topHistory_typeBan', {
                                  defaultValue: 'Ban',
                                })}
                              </option>
                              <option value="kick">
                                {t('adminDashboard.topHistory_typeKick', {
                                  defaultValue: 'Kick',
                                })}
                              </option>
                              <option value="mute">
                                {t('adminDashboard.topHistory_typeMute', {
                                  defaultValue: 'Mute',
                                })}
                              </option>
                            </select>
                            <select
                              className="admin-dashboard__tx-history-select"
                              value={historyAdmin}
                              onChange={(e) => setHistoryAdmin(e.target.value)}
                              aria-label={t(
                                'adminDashboard.topHistory_filterAdmin',
                                { defaultValue: 'Filter by admin' },
                              )}
                            >
                              <option value="any">
                                {t('adminDashboard.topHistory_byAnyAdmin', {
                                  defaultValue: 'By any admin',
                                })}
                              </option>
                              {topbarHistoryData.admins.map((admin) => (
                                <option
                                  key={`history-admin-${admin}`}
                                  value={admin}
                                >
                                  {admin}
                                </option>
                              ))}
                            </select>
                            <button
                              type="button"
                              className="admin-dashboard__tx-history-more"
                              onClick={() => {
                                setHistorySearch('');
                                setHistoryActionType('any');
                                setHistoryAdmin('any');
                              }}
                            >
                              {t('adminDashboard.topHistoryMore', {
                                defaultValue: 'More ▼',
                              })}
                            </button>
                          </div>
                          <p className="admin-dashboard__tx-history-help">
                            {t('adminDashboard.topHistory_help', {
                              defaultValue: 'Search actions by their ID.',
                            })}
                          </p>
                        </div>

                        <div className="admin-dashboard__tx-history-table-wrap">
                          <table className="admin-dashboard__tx-history-table">
                            <thead>
                              <tr>
                                <th>
                                  {t('adminDashboard.topHistory_colAction', {
                                    defaultValue: 'Action',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topHistory_colPlayer', {
                                    defaultValue: 'Player',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topHistory_colReason', {
                                    defaultValue: 'Reason',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topHistory_colAuthor', {
                                    defaultValue: 'Author',
                                  })}
                                </th>
                                <th>
                                  {t('adminDashboard.topHistory_colDateTime', {
                                    defaultValue: 'Date Time ▼',
                                  })}
                                </th>
                              </tr>
                            </thead>
                            <tbody>
                              {filteredHistory.map((entry) => (
                                <tr key={`history-row-${entry.id}`}>
                                  <td>
                                    <span
                                      className={`admin-dashboard__tx-history-badge admin-dashboard__tx-history-badge--${entry.type}`}
                                    >
                                      {entry.type.toUpperCase()}
                                    </span>
                                  </td>
                                  <td>{entry.playerName}</td>
                                  <td>{entry.reason || '-'}</td>
                                  <td>{entry.author || '-'}</td>
                                  <td>{formatAdminTime(entry.ts)}</td>
                                </tr>
                              ))}
                              {filteredHistory.length === 0 && (
                                <tr>
                                  <td colSpan={5}>
                                    {t('adminDashboard.topHistory_noActions', {
                                      defaultValue: 'No actions found.',
                                    })}
                                  </td>
                                </tr>
                              )}
                            </tbody>
                          </table>
                        </div>
                      </section>
                    )}

                    {activeTopSection === 'playerDrops' && (
                      <section className="admin-dashboard__tx-drops">
                        <div className="admin-dashboard__tx-drops-header">
                          <span className="admin-dashboard__tx-drops-header-label">
                            {t('adminDashboard.topDrops_hours', {
                              defaultValue: 'Hours',
                            })}
                          </span>
                          <select
                            className="admin-dashboard__tx-drops-select"
                            value={dropsHoursWindow}
                            onChange={(e) => {
                              const hours = Number(e.target.value);
                              setDropsHoursWindow(hours);
                              void fetchTopbarPlayerDrops(
                                hours,
                                crashReasonsLimit,
                                crashReasonsSortMode,
                              );
                            }}
                            aria-label={t('adminDashboard.topDrops_hours', {
                              defaultValue: 'Hours',
                            })}
                          >
                            <option value={6}>6 Hours</option>
                            <option value={24}>24 Hours</option>
                            <option value={168}>7 Days</option>
                            <option value={720}>30 Days</option>
                          </select>
                        </div>

                        <div className="admin-dashboard__tx-drops-section">
                          <div className="admin-dashboard__tx-drops-section-header">
                            <span
                              className="admin-dashboard__tx-drops-header-icon"
                              aria-hidden="true"
                            >
                              ⬇
                            </span>
                            <span>
                              {t('adminDashboard.topDrops_expected', {
                                defaultValue: 'Expected Player Drops',
                              })}
                            </span>
                          </div>
                          {topbarPlayerDropsData.expected.length === 0 ? (
                            <div className="admin-dashboard__tx-drops-empty">
                              {t('adminDashboard.topDrops_noExpected', {
                                defaultValue:
                                  'No players disconnected from your server recently.',
                              })}
                            </div>
                          ) : (
                            <table className="admin-dashboard__tx-drops-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topDrops_colPlayer', {
                                      defaultValue: 'Player',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colReason', {
                                      defaultValue: 'Reason',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colTime', {
                                      defaultValue: 'Date Time ▼',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarPlayerDropsData.expected.map((entry) => (
                                  <tr
                                    key={`expected-drop-${entry.ts}-${entry.userId}`}
                                  >
                                    <td>
                                      {entry.playerName} ({entry.userId})
                                    </td>
                                    <td>{entry.reason || '-'}</td>
                                    <td>{formatAdminTime(entry.ts)}</td>
                                  </tr>
                                ))}
                              </tbody>
                            </table>
                          )}
                        </div>

                        <div className="admin-dashboard__tx-drops-section">
                          <div className="admin-dashboard__tx-drops-section-header">
                            <span
                              className="admin-dashboard__tx-drops-header-icon"
                              aria-hidden="true"
                            >
                              ⚠
                            </span>
                            <span>
                              {t('adminDashboard.topDrops_unexpected', {
                                defaultValue: 'Unexpected Player Drops',
                              })}
                            </span>
                          </div>
                          {topbarPlayerDropsData.unexpected.length === 0 ? (
                            <div className="admin-dashboard__tx-drops-empty">
                              {t('adminDashboard.topDrops_noUnexpected', {
                                defaultValue:
                                  'No unexpected player drops within this time window.',
                              })}
                            </div>
                          ) : (
                            <table className="admin-dashboard__tx-drops-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topDrops_colPlayer', {
                                      defaultValue: 'Player',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colReason', {
                                      defaultValue: 'Reason',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colTime', {
                                      defaultValue: 'Date Time ▼',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarPlayerDropsData.unexpected.map(
                                  (entry) => (
                                    <tr
                                      key={`unexpected-drop-${entry.ts}-${entry.userId}`}
                                    >
                                      <td>
                                        {entry.playerName} ({entry.userId})
                                      </td>
                                      <td>{entry.reason || '-'}</td>
                                      <td>{formatAdminTime(entry.ts)}</td>
                                    </tr>
                                  ),
                                )}
                              </tbody>
                            </table>
                          )}
                        </div>

                        <div className="admin-dashboard__tx-drops-section admin-dashboard__tx-drops-section--period">
                          <div className="admin-dashboard__tx-drops-section-header">
                            <span
                              className="admin-dashboard__tx-drops-header-icon"
                              aria-hidden="true"
                            >
                              🕒
                            </span>
                            <span>
                              {t('adminDashboard.topDrops_periodOverview', {
                                defaultValue: 'Period Overview',
                              })}
                            </span>
                          </div>
                          <div className="admin-dashboard__tx-drops-period-text">
                            {t('adminDashboard.topDrops_periodText', {
                              defaultValue: 'Period from {{from}} to {{to}}.',
                              from: formatPeriodTs(
                                topbarPlayerDropsData.periodStart,
                              ),
                              to: formatPeriodTs(
                                topbarPlayerDropsData.periodEnd,
                              ),
                            })}
                          </div>
                        </div>

                        <div className="admin-dashboard__tx-drops-section">
                          <div className="admin-dashboard__tx-drops-section-header">
                            <span
                              className="admin-dashboard__tx-drops-header-icon"
                              aria-hidden="true"
                            >
                              🧱
                            </span>
                            <span>
                              {t('adminDashboard.topDrops_resourceKicks', {
                                defaultValue: 'Resource Kicks',
                              })}
                            </span>
                          </div>
                          {topbarPlayerDropsData.resourceKicks.length === 0 ? (
                            <div className="admin-dashboard__tx-drops-empty">
                              {t('adminDashboard.topDrops_noResourceKicks', {
                                defaultValue:
                                  'No players kicked by resources within this time window.',
                              })}
                            </div>
                          ) : (
                            <table className="admin-dashboard__tx-drops-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topDrops_colResource', {
                                      defaultValue: 'Resource',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colCount', {
                                      defaultValue: 'Count',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarPlayerDropsData.resourceKicks.map(
                                  (entry) => (
                                    <tr key={`resource-kick-${entry.resource}`}>
                                      <td>{entry.resource}</td>
                                      <td>{entry.count}</td>
                                    </tr>
                                  ),
                                )}
                              </tbody>
                            </table>
                          )}
                        </div>

                        <div className="admin-dashboard__tx-drops-section">
                          <div className="admin-dashboard__tx-drops-section-header">
                            <span
                              className="admin-dashboard__tx-drops-header-icon"
                              aria-hidden="true"
                            >
                              🌍
                            </span>
                            <span>
                              {t('adminDashboard.topDrops_environmentChanges', {
                                defaultValue: 'Environment Changes',
                              })}
                            </span>
                          </div>
                          {topbarPlayerDropsData.environmentChanges.length ===
                          0 ? (
                            <div className="admin-dashboard__tx-drops-empty">
                              {t('adminDashboard.topDrops_noEnvironment', {
                                defaultValue:
                                  'No environmental changes within this time window.',
                              })}
                            </div>
                          ) : (
                            <table className="admin-dashboard__tx-drops-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topDrops_colType', {
                                      defaultValue: 'Type',
                                    })}
                                  </th>
                                  <th>
                                    {t(
                                      'adminDashboard.topDrops_colDescription',
                                      { defaultValue: 'Description' },
                                    )}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colTime', {
                                      defaultValue: 'Date Time ▼',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarPlayerDropsData.environmentChanges.map(
                                  (entry) => (
                                    <tr
                                      key={`env-change-${entry.ts}-${entry.type}`}
                                    >
                                      <td>{entry.type}</td>
                                      <td>{entry.description}</td>
                                      <td>{formatAdminTime(entry.ts)}</td>
                                    </tr>
                                  ),
                                )}
                              </tbody>
                            </table>
                          )}
                        </div>

                        <div className="admin-dashboard__tx-drops-section">
                          <div className="admin-dashboard__tx-drops-section-header admin-dashboard__tx-drops-section-header--inline">
                            <span className="admin-dashboard__tx-drops-header-inline-title">
                              <span
                                className="admin-dashboard__tx-drops-header-icon"
                                aria-hidden="true"
                              >
                                ⚙
                              </span>
                              <span>
                                {t('adminDashboard.topDrops_crashReasons', {
                                  defaultValue: 'Crash Reasons',
                                })}
                              </span>
                            </span>
                            <div className="admin-dashboard__tx-drops-controls">
                              <select
                                className="admin-dashboard__tx-drops-select"
                                value={crashReasonsLimit}
                                onChange={(e) => {
                                  const nextLimit = Number(e.target.value);
                                  setCrashReasonsLimit(nextLimit);
                                  void fetchTopbarPlayerDrops(
                                    dropsHoursWindow,
                                    nextLimit,
                                    crashReasonsSortMode,
                                  );
                                }}
                                aria-label={t(
                                  'adminDashboard.topDrops_topLimit',
                                  { defaultValue: 'Top limit' },
                                )}
                              >
                                <option value={25}>Top ~25</option>
                                <option value={50}>Top ~50</option>
                                <option value={100}>Top ~100</option>
                                <option value={200}>Top ~200</option>
                              </select>
                              <select
                                className="admin-dashboard__tx-drops-select"
                                value={crashReasonsSortMode}
                                onChange={(e) => {
                                  const sort = e.target.value as
                                    | 'count'
                                    | 'alphabetical';
                                  setCrashReasonsSortMode(sort);
                                  void fetchTopbarPlayerDrops(
                                    dropsHoursWindow,
                                    crashReasonsLimit,
                                    sort,
                                  );
                                }}
                                aria-label={t(
                                  'adminDashboard.topDrops_sortMode',
                                  { defaultValue: 'Sort mode' },
                                )}
                              >
                                <option value="count">
                                  {t('adminDashboard.topDrops_sortByCount', {
                                    defaultValue: 'Sort by Count',
                                  })}
                                </option>
                                <option value="alphabetical">
                                  {t(
                                    'adminDashboard.topDrops_sortByAlphabetical',
                                    { defaultValue: 'Alphabetical' },
                                  )}
                                </option>
                              </select>
                            </div>
                          </div>
                          {topbarPlayerDropsData.crashReasons.length === 0 ? (
                            <div className="admin-dashboard__tx-drops-empty">
                              {t('adminDashboard.topDrops_noCrashReasons', {
                                defaultValue:
                                  'No crash reasons within this time window.',
                              })}
                            </div>
                          ) : (
                            <table className="admin-dashboard__tx-drops-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topDrops_colReason', {
                                      defaultValue: 'Reason',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topDrops_colCount', {
                                      defaultValue: 'Count',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarPlayerDropsData.crashReasons.map(
                                  (entry) => (
                                    <tr key={`crash-reason-${entry.reason}`}>
                                      <td>{entry.reason}</td>
                                      <td>{entry.count}</td>
                                    </tr>
                                  ),
                                )}
                              </tbody>
                            </table>
                          )}
                        </div>
                      </section>
                    )}

                    {activeTopSection === 'whitelist' && (
                      <section className="admin-dashboard__tx-whitelist">
                        {whitelistNotInApprovedLicenseMode && (
                          <div className="admin-dashboard__tx-whitelist-warning">
                            <div className="admin-dashboard__tx-whitelist-warning-title">
                              {t('adminDashboard.topWhitelist_warningTitle', {
                                defaultValue:
                                  'Warning: The server is not in the "Approved License" whitelist mode.',
                              })}
                            </div>
                            <div>
                              {t('adminDashboard.topWhitelist_warningText1', {
                                defaultValue:
                                  'This means that any changes made in this page will not reflect in the ability of players to connect to the server while the whitelist is in another mode or disabled.',
                              })}
                            </div>
                            <div>
                              {t('adminDashboard.topWhitelist_warningText2', {
                                defaultValue:
                                  'This can be changed in the page Settings > Player Manager.',
                              })}
                            </div>
                          </div>
                        )}

                        <div className="admin-dashboard__tx-whitelist-grid">
                          <article className="admin-dashboard__tx-whitelist-panel">
                            <div className="admin-dashboard__tx-whitelist-panel-head">
                              <div>
                                <h4>
                                  {t(
                                    'adminDashboard.topWhitelist_requestsTitle',
                                    { defaultValue: 'Whitelist Requests' },
                                  )}
                                  : 0
                                </h4>
                                <p>
                                  {t(
                                    'adminDashboard.topWhitelist_requestsDesc',
                                    {
                                      defaultValue:
                                        'Players that tried to join the server but were not whitelisted.',
                                    },
                                  )}
                                </p>
                              </div>
                              <div className="admin-dashboard__tx-whitelist-search-wrap">
                                <input
                                  type="text"
                                  className="admin-dashboard__tx-whitelist-search"
                                  value={topbarWhitelistSearch}
                                  onChange={(e) =>
                                    setTopbarWhitelistSearch(e.target.value)
                                  }
                                  placeholder={t(
                                    'adminDashboard.topWhitelist_searchPlaceholder',
                                    { defaultValue: 'player name, R1234' },
                                  )}
                                />
                                <button
                                  type="button"
                                  className="admin-dashboard__tx-whitelist-search-btn"
                                >
                                  {t('adminDashboard.search', {
                                    defaultValue: 'Search',
                                  })}
                                </button>
                              </div>
                            </div>
                            <div className="admin-dashboard__tx-whitelist-panel-body admin-dashboard__tx-whitelist-panel-body--empty">
                              {t('adminDashboard.topWhitelist_noPlayers', {
                                defaultValue: 'no players here yet',
                              })}
                            </div>
                            <div className="admin-dashboard__tx-whitelist-panel-foot">
                              <button
                                type="button"
                                className="admin-dashboard__tx-whitelist-deny"
                                disabled
                              >
                                {t('adminDashboard.topWhitelist_denyAll', {
                                  defaultValue: 'Deny All',
                                })}
                              </button>
                              <div className="admin-dashboard__tx-whitelist-pagination">
                                <button type="button" disabled>
                                  {t('adminDashboard.previous', {
                                    defaultValue: 'Previous',
                                  })}
                                </button>
                                <span>1</span>
                                <button type="button" disabled>
                                  {t('adminDashboard.next', {
                                    defaultValue: 'Next',
                                  })}
                                </button>
                              </div>
                            </div>
                          </article>

                          <article className="admin-dashboard__tx-whitelist-panel">
                            <div className="admin-dashboard__tx-whitelist-panel-head">
                              <div>
                                <h4>
                                  {t(
                                    'adminDashboard.topWhitelist_approvedTitle',
                                    {
                                      defaultValue:
                                        'Approved Whitelists Pending Join',
                                    },
                                  )}
                                  : {whitelistPendingJoinCount}
                                </h4>
                                <p>
                                  {t(
                                    'adminDashboard.topWhitelist_approvedDesc',
                                    {
                                      defaultValue:
                                        "Players that are already approved, but haven't joined the server yet.",
                                    },
                                  )}
                                </p>
                              </div>
                              <button
                                type="button"
                                className="admin-dashboard__tx-whitelist-approve"
                                onClick={() => {
                                  setTopbarWhitelistModalIdentifier('');
                                  setTopbarWhitelistModalOpen(true);
                                }}
                              >
                                +{' '}
                                {t('adminDashboard.topWhitelist_addApproval', {
                                  defaultValue: 'Add Approval',
                                })}
                              </button>
                            </div>
                            <div className="admin-dashboard__tx-whitelist-panel-body">
                              {whitelistApprovedEntries.length === 0 ? (
                                <div className="admin-dashboard__tx-whitelist-panel-body--empty">
                                  {t('adminDashboard.topWhitelist_noPlayers', {
                                    defaultValue: 'no players here yet',
                                  })}
                                </div>
                              ) : (
                                <table className="admin-dashboard__tx-whitelist-table">
                                  <thead>
                                    <tr>
                                      <th>
                                        {t(
                                          'adminDashboard.topWhitelist_colSource',
                                          { defaultValue: 'Source' },
                                        )}
                                      </th>
                                      <th>
                                        {t(
                                          'adminDashboard.topWhitelist_colValue',
                                          { defaultValue: 'Identifier' },
                                        )}
                                      </th>
                                      <th>
                                        {t(
                                          'adminDashboard.topWhitelist_colMode',
                                          { defaultValue: 'Mode' },
                                        )}
                                      </th>
                                    </tr>
                                  </thead>
                                  <tbody>
                                    {whitelistApprovedEntries
                                      .slice(0, 50)
                                      .map((entry) => (
                                        <tr key={entry.id}>
                                          <td>{entry.source}</td>
                                          <td>{entry.value}</td>
                                          <td>{whitelistModeLabel}</td>
                                        </tr>
                                      ))}
                                  </tbody>
                                </table>
                              )}
                            </div>
                          </article>
                        </div>

                        {/* Whitelist Add Modal */}
                        {topbarWhitelistModalOpen && (
                          <div
                            className="admin-dashboard__tx-whitelist-modal-backdrop"
                            onClick={() =>
                              !topbarWhitelistModalSaving &&
                              setTopbarWhitelistModalOpen(false)
                            }
                          >
                            <div
                              className="admin-dashboard__tx-whitelist-modal"
                              onClick={(e) => e.stopPropagation()}
                            >
                              <div className="admin-dashboard__tx-whitelist-modal-head">
                                <h5>
                                  {t('adminDashboard.topWhitelist_addPlayer', {
                                    defaultValue: 'Whitelist player',
                                  })}
                                </h5>
                                <button
                                  type="button"
                                  onClick={() =>
                                    setTopbarWhitelistModalOpen(false)
                                  }
                                  disabled={topbarWhitelistModalSaving}
                                >
                                  ×
                                </button>
                              </div>

                              <div className="admin-dashboard__tx-whitelist-modal-body">
                                <p>
                                  {t(
                                    'adminDashboard.topWhitelist_addPlayerDescription',
                                    {
                                      defaultValue:
                                        'Type in the Player Identifier you want to whitelist.',
                                    },
                                  )}
                                </p>
                                <p className="admin-dashboard__tx-whitelist-modal-hint">
                                  {t(
                                    'adminDashboard.topWhitelist_identifierTypes',
                                    {
                                      defaultValue:
                                        'This can be any of the following: discord, steam, license, licenseea, live, xblive, fal.',
                                    },
                                  )}
                                </p>
                                <label>
                                  <input
                                    type="text"
                                    value={topbarWhitelistModalIdentifier}
                                    aria-invalid={
                                      topbarWhitelistModalIdentifier.trim()
                                        ? !isValidWhitelistIdentifier(
                                            topbarWhitelistModalIdentifier,
                                          )
                                        : false
                                    }
                                    onChange={(e) =>
                                      setTopbarWhitelistModalIdentifier(
                                        e.target.value,
                                      )
                                    }
                                    placeholder={t(
                                      'adminDashboard.topWhitelist_identifierPlaceholder',
                                      {
                                        defaultValue:
                                          'discord:272600190639848628',
                                      },
                                    )}
                                    disabled={topbarWhitelistModalSaving}
                                    autoFocus
                                  />
                                </label>
                              </div>

                              <div className="admin-dashboard__tx-whitelist-modal-foot">
                                <button
                                  type="button"
                                  onClick={() =>
                                    setTopbarWhitelistModalOpen(false)
                                  }
                                  disabled={topbarWhitelistModalSaving}
                                >
                                  {t('adminDashboard.cancel', {
                                    defaultValue: 'Cancel',
                                  })}
                                </button>
                                <button
                                  type="button"
                                  className="admin-dashboard__tx-whitelist-submit"
                                  onClick={() => saveTopbarWhitelistEntry()}
                                  disabled={
                                    topbarWhitelistModalSaving ||
                                    !topbarWhitelistModalIdentifier.trim() ||
                                    !isValidWhitelistIdentifier(
                                      topbarWhitelistModalIdentifier,
                                    )
                                  }
                                >
                                  {topbarWhitelistModalSaving
                                    ? t('adminDashboard.saving', {
                                        defaultValue: 'Saving...',
                                      })
                                    : t('adminDashboard.submit', {
                                        defaultValue: 'Submit',
                                      })}
                                </button>
                              </div>
                            </div>
                          </div>
                        )}
                      </section>
                    )}

                    {activeTopSection === 'admins' && (
                      <section className="admin-dashboard__tx-admins">
                        <article className="admin-dashboard__tx-admins-panel">
                          <div className="admin-dashboard__tx-admins-head">
                            <h4>
                              {t('adminDashboard.topAdmins_allAdmins', {
                                defaultValue: 'All Admins',
                              })}{' '}
                              ({topbarAdminsData.entries.length})
                            </h4>
                            <button
                              type="button"
                              className="admin-dashboard__tx-admins-add"
                              onClick={openTopbarAdminCreateModal}
                            >
                              +{' '}
                              {t('adminDashboard.topAdmins_add', {
                                defaultValue: 'Add',
                              })}
                            </button>
                          </div>

                          <div className="admin-dashboard__tx-admins-table-wrap">
                            <table className="admin-dashboard__tx-admins-table">
                              <thead>
                                <tr>
                                  <th>
                                    {t('adminDashboard.topAdmins_colUsername', {
                                      defaultValue: 'Username',
                                    })}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topAdmins_colAuth', {
                                      defaultValue: 'Auth',
                                    })}
                                  </th>
                                  <th>
                                    {t(
                                      'adminDashboard.topAdmins_colPermissions',
                                      { defaultValue: 'Permissions' },
                                    )}
                                  </th>
                                  <th>
                                    {t('adminDashboard.topAdmins_colActions', {
                                      defaultValue: 'Actions',
                                    })}
                                  </th>
                                </tr>
                              </thead>
                              <tbody>
                                {topbarAdminsData.entries.map((entry) => (
                                  <tr
                                    key={`top-admin-row-${entry.user}`}
                                    className={
                                      entry.isPrimary
                                        ? 'admin-dashboard__tx-admins-row--master'
                                        : ''
                                    }
                                  >
                                    <td>
                                      <span className="admin-dashboard__tx-admins-user-cell">
                                        {entry.isPrimary && (
                                          <span
                                            className="admin-dashboard__tx-admins-master-badge"
                                            title={t(
                                              'adminDashboard.topAdmins_masterTooltip',
                                              {
                                                defaultValue:
                                                  'Master Admin – cannot be deleted or demoted',
                                              },
                                            )}
                                          >
                                            &#128081;
                                          </span>
                                        )}
                                        {entry.user}
                                      </span>
                                    </td>
                                    <td>
                                      <span className="admin-dashboard__tx-admins-auth-icons">
                                        {entry.auth.password && (
                                          <span
                                            className="admin-dashboard__tx-admins-auth admin-dashboard__tx-admins-auth--password"
                                            title="Password login"
                                          >
                                            🔑
                                          </span>
                                        )}
                                        {entry.auth.discord && (
                                          <span
                                            className="admin-dashboard__tx-admins-auth admin-dashboard__tx-admins-auth--discord"
                                            title="Discord ID"
                                          >
                                            ☍
                                          </span>
                                        )}
                                      </span>
                                    </td>
                                    <td>
                                      {entry.isPrimary ? (
                                        <span className="admin-dashboard__tx-admins-master-label">
                                          {t(
                                            'adminDashboard.topAdmins_masterAccount',
                                            { defaultValue: 'Master Admin' },
                                          )}
                                        </span>
                                      ) : (
                                        entry.permissionsLabel
                                      )}
                                    </td>
                                    <td>
                                      <div className="admin-dashboard__tx-admins-actions">
                                        {entry.isCurrentUser ? (
                                          <span className="admin-dashboard__tx-admins-your-account">
                                            {t(
                                              'adminDashboard.topAdmins_yourAccount',
                                              { defaultValue: 'Your Account' },
                                            )}
                                          </span>
                                        ) : (
                                          <>
                                            <button
                                              type="button"
                                              className="admin-dashboard__tx-admins-edit"
                                              onClick={() =>
                                                openTopbarAdminEditModal(entry)
                                              }
                                            >
                                              {t('adminDashboard.edit', {
                                                defaultValue: 'Edit',
                                              })}
                                            </button>
                                            <button
                                              type="button"
                                              className="admin-dashboard__tx-admins-delete"
                                              onClick={() => {
                                                void deleteTopbarAdminUser(
                                                  entry.user,
                                                );
                                              }}
                                            >
                                              {t('adminDashboard.delete', {
                                                defaultValue: 'Delete',
                                              })}
                                            </button>
                                          </>
                                        )}
                                      </div>
                                    </td>
                                  </tr>
                                ))}
                              </tbody>
                            </table>
                          </div>
                        </article>

                        {topbarAdminModalOpen && (
                          <div
                            className="admin-dashboard__tx-admins-modal-backdrop"
                            onClick={() =>
                              !topbarAdminSaving &&
                              setTopbarAdminModalOpen(false)
                            }
                          >
                            <div
                              className="admin-dashboard__tx-admins-modal"
                              onClick={(e) => e.stopPropagation()}
                            >
                              <div className="admin-dashboard__tx-admins-modal-head">
                                <h5>
                                  {topbarAdminEditingUser
                                    ? t('adminDashboard.topAdmins_editAdmin', {
                                        defaultValue: 'Edit Admin',
                                      })
                                    : t('adminDashboard.topAdmins_newAdmin', {
                                        defaultValue: 'New Admin',
                                      })}
                                </h5>
                                <button
                                  type="button"
                                  onClick={() => setTopbarAdminModalOpen(false)}
                                  disabled={topbarAdminSaving}
                                >
                                  ×
                                </button>
                              </div>

                              <div className="admin-dashboard__tx-admins-modal-body">
                                <label>
                                  <span>
                                    {t('adminDashboard.topAdmins_colUsername', {
                                      defaultValue: 'Username',
                                    })}
                                  </span>
                                  <input
                                    type="text"
                                    value={topbarAdminForm.user}
                                    disabled={Boolean(topbarAdminEditingUser)}
                                    onChange={(e) =>
                                      setTopbarAdminForm((prev) => ({
                                        ...prev,
                                        user: e.target.value,
                                      }))
                                    }
                                    placeholder="username"
                                  />
                                </label>
                                <label>
                                  <span>
                                    {t('adminDashboard.topAdmins_discordId', {
                                      defaultValue: 'Discord ID (optional)',
                                    })}
                                  </span>
                                  <input
                                    type="text"
                                    value={topbarAdminForm.discordId}
                                    aria-invalid={
                                      !isValidDiscordSnowflake(
                                        topbarAdminForm.discordId,
                                      )
                                    }
                                    onChange={(e) =>
                                      setTopbarAdminForm((prev) => ({
                                        ...prev,
                                        discordId: e.target.value,
                                      }))
                                    }
                                    placeholder="123456789012345678"
                                  />
                                </label>
                                <label>
                                  <span>
                                    {t('adminDashboard.role', {
                                      defaultValue: 'Role',
                                    })}
                                  </span>
                                  <select
                                    value={topbarAdminForm.role}
                                    onChange={(e) =>
                                      setTopbarAdminForm((prev) => ({
                                        ...prev,
                                        role: e.target.value as AdminRole,
                                      }))
                                    }
                                  >
                                    <option value="admin">
                                      {t('adminDashboard.role_admin')}
                                    </option>
                                    <option value="moderator">
                                      {t('adminDashboard.role_moderator')}
                                    </option>
                                    <option value="viewer">
                                      {t('adminDashboard.role_viewer')}
                                    </option>
                                  </select>
                                </label>
                                <p className="admin-dashboard__tx-admins-modal-note">
                                  {t(
                                    'adminDashboard.topAdmins_discordLoginNote',
                                    {
                                      defaultValue:
                                        'Discord IDs can use the Discord button on the regular admin login page once OAuth is configured.',
                                    },
                                  )}
                                </p>
                              </div>

                              <div className="admin-dashboard__tx-admins-modal-foot">
                                <button
                                  type="button"
                                  onClick={() => setTopbarAdminModalOpen(false)}
                                  disabled={topbarAdminSaving}
                                >
                                  {t('adminDashboard.cancel', {
                                    defaultValue: 'Cancel',
                                  })}
                                </button>
                                <button
                                  type="button"
                                  className="admin-dashboard__tx-admins-save"
                                  onClick={() => {
                                    void saveTopbarAdminUser();
                                  }}
                                  disabled={
                                    topbarAdminSaving ||
                                    !isValidDiscordSnowflake(
                                      topbarAdminForm.discordId,
                                    )
                                  }
                                >
                                  {topbarAdminSaving
                                    ? t('adminDashboard.saving', {
                                        defaultValue: 'Saving...',
                                      })
                                    : t('adminDashboard.save', {
                                        defaultValue: 'Save',
                                      })}
                                </button>
                              </div>
                            </div>
                          </div>
                        )}
                      </section>
                    )}

                    {activeTopSection === 'settings' && (
                      <section
                        className="admin-dashboard__panel"
                        id="admin-top-panel-settings-redirect"
                        role="region"
                        aria-label={t('adminDashboard.settings', {
                          defaultValue: 'Settings',
                        })}
                      >
                        <div className="admin-dashboard__panel-toolbar">
                          <div className="admin-dashboard__panel-toolbar-left">
                            <span className="admin-dashboard__panel-toolbar-title">
                              {t('adminDashboard.settings', {
                                defaultValue: 'Settings',
                              })}
                            </span>
                          </div>
                        </div>
                        <p className="admin-dashboard__cfg-status">
                          {t('adminDashboard.settingsUnifiedEditorHint', {
                            defaultValue:
                              'Settings are managed in the unified Configuration Editor.',
                          })}
                        </p>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn admin-dashboard__log-btn--primary"
                          onClick={() => {
                            setActiveMenuSurface('sidebar');
                            setActiveTab('cfg');
                            setCfgEditorTab('general');
                          }}
                        >
                          {t('adminDashboard.openCfgEditor', {
                            defaultValue: 'Open Configuration Editor',
                          })}
                        </button>
                      </section>
                    )}
                  </div>
                </section>
              )}

            {activeMenuSurface === 'sidebar' && (
              <div className="admin-dashboard__summary-grid">
                {summaryCards.map((card) => (
                  <div
                    key={card.label}
                    className={`admin-dashboard__summary-card${
                      card.tone === 'warn'
                        ? ' admin-dashboard__summary-card--warn'
                        : ''
                    }`}
                  >
                    <div className="admin-dashboard__summary-label">
                      {card.label}
                    </div>
                    <div
                      className={`admin-dashboard__summary-value${
                        card.tone === 'accent'
                          ? ' admin-dashboard__summary-value--accent'
                          : ''
                      }${
                        card.tone === 'warn'
                          ? ' admin-dashboard__summary-value--warn'
                          : ''
                      }`}
                    >
                      {card.value}
                    </div>
                  </div>
                ))}
              </div>
            )}

            {loading && (
              <div className="admin-dashboard__loading">
                {t('adminDashboard.loading')}
              </div>
            )}

            {!loading && (
              <>
                {activeMenuSurface === 'sidebar' &&
                  activeTab === 'overview' &&
                  status && (
                    <div
                      className="admin-dashboard__panel"
                      id="admin-panel-overview"
                      role="tabpanel"
                      aria-labelledby="admin-tab-overview"
                    >
                      <div className="admin-dashboard__overview-info">
                        {[
                          {
                            k: t('adminDashboard.serverName'),
                            v: status.name || '-',
                          },
                          {
                            k: t('adminDashboard.port'),
                            v: String(status.port),
                          },
                          {
                            k: t('adminDashboard.uptime'),
                            v: formatAdminUptime(status.uptimeSec),
                          },
                          {
                            k: t('adminDashboard.online'),
                            v: `${status.online} / ${status.maxPlayers}`,
                          },
                          {
                            k: t('adminDashboard.cfgItemPickupMode'),
                            v: t(
                              `adminDashboard.cfgItemPickupMode_${
                                status.itemPickupMode === 'minigame'
                                  ? 'minigame'
                                  : status.itemPickupMode === 'block-all'
                                    ? 'blockAll'
                                    : 'allow'
                              }`,
                            ),
                          },
                          { k: t('adminDashboard.user'), v: adminUser || '-' },
                          {
                            k: t('adminDashboard.role'),
                            v: t(`adminDashboard.role_${adminRole}`),
                          },
                        ].map(({ k, v }) => (
                          <div key={k} className="admin-dashboard__info-row">
                            <span className="admin-dashboard__info-key">
                              {k}
                            </span>
                            <span className="admin-dashboard__info-val">
                              {v}
                            </span>
                          </div>
                        ))}
                      </div>

                      <div className="admin-dashboard__capabilities">
                        <h3 className="admin-dashboard__section-subtitle">
                          {t('adminDashboard.permissionsTitle')}
                        </h3>
                        <div className="admin-dashboard__capabilities-list">
                          {capabilityRows.map(({ key, label }) => {
                            const enabled = capabilities[key];
                            return (
                              <div
                                key={key}
                                className="admin-dashboard__capability-row"
                              >
                                <span className="admin-dashboard__capability-label">
                                  {label}
                                </span>
                                <span
                                  className={`admin-dashboard__capability-state ${
                                    enabled ? 'is-enabled' : 'is-disabled'
                                  }`}
                                >
                                  {enabled
                                    ? t('adminDashboard.capabilityEnabled')
                                    : t('adminDashboard.capabilityDisabled')}
                                </span>
                              </div>
                            );
                          })}
                        </div>
                      </div>
                    </div>
                  )}

                {activeMenuSurface === 'sidebar' && activeTab === 'players' && (
                  <div
                    className="admin-dashboard__panel"
                    id="admin-panel-players"
                    data-testid="admin-panel-players"
                    role="tabpanel"
                    aria-labelledby="admin-tab-players"
                  >
                    <div className="admin-dashboard__panel-toolbar">
                      <div className="admin-dashboard__panel-toolbar-left">
                        <span className="admin-dashboard__panel-toolbar-title">
                          {t('adminDashboard.tabPlayers')}
                        </span>
                        {filteredPlayers.length > 0 && (
                          <span className="admin-dashboard__panel-badge">
                            {filteredPlayers.length}
                          </span>
                        )}
                      </div>
                    </div>
                    <div className="admin-dashboard__search-row">
                      <input
                        className="admin-dashboard__search-input"
                        type="text"
                        placeholder={t('adminDashboard.searchPlaceholder')}
                        aria-label={t('adminDashboard.searchPlaceholder')}
                        value={playerSearch}
                        onChange={(e) => setPlayerSearch(e.target.value)}
                      />
                      <input
                        className="admin-dashboard__search-input admin-dashboard__search-input--reason"
                        data-testid="admin-reason-input"
                        type="text"
                        placeholder={t('adminDashboard.reasonPlaceholder')}
                        aria-label={t('adminDashboard.reasonLabel')}
                        aria-invalid={moderationReasonInvalid}
                        value={moderationReason}
                        onChange={(e) => setModerationReason(e.target.value)}
                        maxLength={MAX_MODERATION_REASON_LENGTH + 1}
                      />
                      <label className="admin-dashboard__mute-duration">
                        <span>{t('adminDashboard.muteDuration')}</span>
                        <select
                          className="admin-dashboard__log-select"
                          data-testid="admin-mute-duration-select"
                          value={muteDurationMinutes}
                          onChange={(e) =>
                            setMuteDurationMinutes(Number(e.target.value))
                          }
                          aria-label={t('adminDashboard.muteDuration')}
                        >
                          {[5, 10, 15, 30, 60, 180, 720].map((value) => (
                            <option key={value} value={value}>
                              {value}m
                            </option>
                          ))}
                        </select>
                      </label>
                      <label className="admin-dashboard__mute-duration">
                        <span>{t('adminDashboard.banDuration')}</span>
                        <select
                          className="admin-dashboard__log-select"
                          data-testid="admin-ban-duration-select"
                          value={banDurationMinutes}
                          onChange={(e) =>
                            setBanDurationMinutes(Number(e.target.value))
                          }
                          aria-label={t('adminDashboard.banDuration')}
                        >
                          <option value={0}>
                            {t('adminDashboard.banPermanent')}
                          </option>
                          {[60, 180, 720, 1440, 4320, 10080, 43200].map(
                            (value) => (
                              <option key={value} value={value}>
                                {value >= 1440
                                  ? `${value / 1440}d`
                                  : `${value}m`}
                              </option>
                            ),
                          )}
                        </select>
                      </label>
                    </div>

                    {filteredPlayers.length === 0 ? (
                      <p className="admin-dashboard__no-players">
                        {players.length === 0
                          ? t('adminDashboard.noPlayers')
                          : t('adminDashboard.noMatch')}
                      </p>
                    ) : (
                      <div
                        className="admin-dashboard__table-wrapper"
                        data-testid="admin-players-table"
                      >
                        <table className="admin-dashboard__table">
                          <caption className="admin-dashboard__sr-only">
                            {t('adminDashboard.tabPlayers')}
                          </caption>
                          <thead>
                            <tr>
                              <th scope="col">{t('adminDashboard.userId')}</th>
                              <th scope="col">{t('adminDashboard.actorId')}</th>
                              <th scope="col">{t('adminDashboard.name')}</th>
                              <th scope="col">{t('adminDashboard.ip')}</th>
                              <th scope="col">{t('adminDashboard.pos')}</th>
                              <th scope="col">{t('adminDashboard.actions')}</th>
                            </tr>
                          </thead>
                          <tbody>
                            {filteredPlayers.map((player) => {
                              const muteEntry = activeMuteByUserId.get(
                                player.userId,
                              );
                              const remainingSec = muteEntry
                                ? Math.max(
                                    0,
                                    Math.floor(
                                      (muteEntry.expiresAt - nowTs) / 1000,
                                    ),
                                  )
                                : 0;
                              const banEntry = activeBanByUserId.get(
                                player.userId,
                              );
                              const banRemainingSec =
                                banEntry &&
                                !banEntry.permanent &&
                                banEntry.expiresAt !== null
                                  ? Math.max(
                                      0,
                                      Math.floor(
                                        (banEntry.expiresAt - nowTs) / 1000,
                                      ),
                                    )
                                  : null;
                              return (
                                <tr
                                  key={player.userId}
                                  data-testid={`admin-player-row-${player.userId}`}
                                >
                                  <td>{player.userId}</td>
                                  <td>{player.actorId || '-'}</td>
                                  <td>
                                    <span>{player.actorName || '-'}</span>
                                    {muteEntry && (
                                      <span
                                        className="admin-dashboard__muted-badge"
                                        data-testid={`admin-muted-badge-${player.userId}`}
                                        title={t('adminDashboard.muted')}
                                      >
                                        {t('adminDashboard.muted')}:{' '}
                                        {formatAdminUptime(remainingSec)}
                                      </span>
                                    )}
                                    {banEntry && (
                                      <span
                                        className="admin-dashboard__banned-badge"
                                        data-testid={`admin-banned-badge-${player.userId}`}
                                        title={t('adminDashboard.banned')}
                                      >
                                        {t('adminDashboard.banned')}
                                        {banRemainingSec !== null
                                          ? `: ${formatAdminUptime(
                                              banRemainingSec,
                                            )}`
                                          : ''}
                                      </span>
                                    )}
                                  </td>
                                  <td>{player.ip}</td>
                                  <td className="admin-dashboard__pos">
                                    {formatAdminPos(player.pos)}
                                  </td>
                                  <td className="admin-dashboard__actions-cell">
                                    <button
                                      type="button"
                                      className="admin-dashboard__kick-btn"
                                      onClick={() => kickPlayer(player.userId)}
                                      disabled={!capabilities.canKick}
                                      title={
                                        !capabilities.canKick
                                          ? t('adminDashboard.noPermission')
                                          : undefined
                                      }
                                    >
                                      {t('adminDashboard.kick')}
                                    </button>
                                    <button
                                      type="button"
                                      className="admin-dashboard__ban-btn"
                                      onClick={() => banPlayer(player.userId)}
                                      disabled={!capabilities.canBan}
                                      title={
                                        !capabilities.canBan
                                          ? t('adminDashboard.noPermission')
                                          : undefined
                                      }
                                    >
                                      {t('adminDashboard.ban')}
                                    </button>
                                    <button
                                      type="button"
                                      className="admin-dashboard__mute-btn"
                                      data-testid={`admin-mute-btn-${player.userId}`}
                                      onClick={() => mutePlayer(player.userId)}
                                      disabled={!capabilities.canMute}
                                      title={
                                        !capabilities.canMute
                                          ? t('adminDashboard.noPermission')
                                          : undefined
                                      }
                                    >
                                      {t('adminDashboard.mute')}
                                    </button>
                                    <button
                                      type="button"
                                      className="admin-dashboard__msg-btn"
                                      data-testid={`admin-msg-btn-${player.userId}`}
                                      onClick={() => {
                                        setSendMsgTargetId(player.userId);
                                        setSendMsgTargetName(
                                          player.actorName ||
                                            String(player.userId),
                                        );
                                        setSendMsgText('');
                                        setStatusMsg('');
                                      }}
                                      disabled={!capabilities.canMessage}
                                      title={
                                        !capabilities.canMessage
                                          ? t('adminDashboard.noPermission')
                                          : undefined
                                      }
                                    >
                                      {t('adminDashboard.messageBtn')}
                                    </button>
                                    <button
                                      type="button"
                                      className="admin-dashboard__msg-btn"
                                      onClick={() =>
                                        void showPlayerInventory(
                                          player.userId,
                                          player.actorName,
                                        )
                                      }
                                    >
                                      {t('adminDashboard.inventoryBtn', {
                                        defaultValue: 'Inventory',
                                      })}
                                    </button>
                                  </td>
                                </tr>
                              );
                            })}
                          </tbody>
                        </table>
                      </div>
                    )}

                    {(capabilities.canBan || capabilities.canUnban) && (
                      <div className="admin-dashboard__bans-section">
                        <h3 className="admin-dashboard__section-subtitle">
                          {t('adminDashboard.bannedUsers')}
                        </h3>
                        {activeBannedUsers.length === 0 ? (
                          <p className="admin-dashboard__bans-empty">
                            {t('adminDashboard.noBans')}
                          </p>
                        ) : (
                          <div className="admin-dashboard__bans-list">
                            {activeBannedUsers.map((entry) => {
                              const remainingBanSec = entry.permanent
                                ? null
                                : Math.max(
                                    0,
                                    Math.floor(
                                      ((entry.expiresAt ?? 0) - nowTs) / 1000,
                                    ),
                                  );
                              return (
                                <div
                                  key={entry.userId}
                                  className="admin-dashboard__ban-row"
                                  data-testid={`admin-banned-row-${entry.userId}`}
                                >
                                  <span className="admin-dashboard__ban-user">
                                    userId: {entry.userId}
                                    {entry.permanent
                                      ? ` • ${t('adminDashboard.banPermanent')}`
                                      : remainingBanSec !== null
                                      ? ` • ${t(
                                          'adminDashboard.banRemaining',
                                        )}: ${formatAdminUptime(
                                          remainingBanSec,
                                        )}`
                                      : ''}
                                  </span>
                                  <button
                                    type="button"
                                    className="admin-dashboard__unban-btn"
                                    data-testid={`admin-unban-btn-${entry.userId}`}
                                    onClick={() => unbanPlayer(entry.userId)}
                                    disabled={!capabilities.canUnban}
                                    title={
                                      !capabilities.canUnban
                                        ? t('adminDashboard.noPermission')
                                        : undefined
                                    }
                                  >
                                    {t('adminDashboard.unban')}
                                  </button>
                                </div>
                              );
                            })}
                          </div>
                        )}
                      </div>
                    )}

                    {(capabilities.canMute || capabilities.canUnmute) && (
                      <div
                        className="admin-dashboard__bans-section"
                        data-testid="admin-muted-users-section"
                      >
                        <h3 className="admin-dashboard__section-subtitle">
                          {t('adminDashboard.mutedUsers')}
                        </h3>
                        {activeMutedUsers.length === 0 ? (
                          <p className="admin-dashboard__bans-empty">
                            {t('adminDashboard.noMutes')}
                          </p>
                        ) : (
                          <div className="admin-dashboard__bans-list">
                            {activeMutedUsers.map((entry) => (
                              <div
                                key={entry.userId}
                                className="admin-dashboard__ban-row"
                                data-testid={`admin-muted-row-${entry.userId}`}
                              >
                                <span className="admin-dashboard__ban-user">
                                  userId: {entry.userId} •{' '}
                                  {t('adminDashboard.muteRemaining')}:{' '}
                                  {formatAdminUptime(
                                    Math.max(
                                      0,
                                      Math.floor(
                                        (entry.expiresAt - nowTs) / 1000,
                                      ),
                                    ),
                                  )}
                                </span>
                                <button
                                  type="button"
                                  className="admin-dashboard__unban-btn"
                                  data-testid={`admin-unmute-btn-${entry.userId}`}
                                  onClick={() => unmutePlayer(entry.userId)}
                                  disabled={!capabilities.canUnmute}
                                  title={
                                    !capabilities.canUnmute
                                      ? t('adminDashboard.noPermission')
                                      : undefined
                                  }
                                >
                                  {t('adminDashboard.unmute')}
                                </button>
                              </div>
                            ))}
                          </div>
                        )}
                      </div>
                    )}

                    {inventoryTargetUserId !== null && (
                      <div className="admin-dashboard__bans-section">
                        <h3 className="admin-dashboard__section-subtitle">
                          {t('adminDashboard.inventoryTitle', {
                            defaultValue: 'Inventory',
                          })}
                          : {inventoryTargetName}
                        </h3>

                        {inventoryLoading && (
                          <p className="admin-dashboard__bans-empty">
                            {t('adminDashboard.loading')}
                          </p>
                        )}

                        {!inventoryLoading && inventoryError && (
                          <p className="admin-dashboard__bans-empty">
                            {inventoryError}
                          </p>
                        )}

                        {!inventoryLoading &&
                          !inventoryError &&
                          playerInventory && (
                            <>
                              <div className="admin-dashboard__inventory-meta">
                                <span>
                                  {t('adminDashboard.inventorySource', {
                                    defaultValue: 'Source',
                                  })}
                                  : {playerInventory.source}
                                </span>
                                {typeof playerInventory.profileId ===
                                  'number' && (
                                  <span>
                                    {t('adminDashboard.inventoryProfileId', {
                                      defaultValue: 'Profile ID',
                                    })}
                                    : {playerInventory.profileId}
                                  </span>
                                )}
                                {playerInventory.formDesc && (
                                  <span>
                                    {t('adminDashboard.inventoryFormDesc', {
                                      defaultValue: 'Form',
                                    })}
                                    : {playerInventory.formDesc}
                                  </span>
                                )}
                                {typeof playerInventory.updatedAt ===
                                  'number' && (
                                  <span>
                                    {t('adminDashboard.inventoryUpdatedAt', {
                                      defaultValue: 'Updated',
                                    })}
                                    :{' '}
                                    {formatAdminTime(playerInventory.updatedAt)}
                                  </span>
                                )}
                              </div>

                              {selectedInventoryEntries.length === 0 ? (
                                <p className="admin-dashboard__bans-empty">
                                  {t('adminDashboard.inventoryEmpty', {
                                    defaultValue: 'No inventory entries',
                                  })}
                                </p>
                              ) : (
                                <div className="admin-dashboard__table-wrapper">
                                  <table className="admin-dashboard__table">
                                    <thead>
                                      <tr>
                                        <th scope="col">
                                          {t('adminDashboard.cfgCode')}
                                        </th>
                                        <th scope="col">
                                          {t('adminDashboard.cfgItem')}
                                        </th>
                                        <th scope="col">
                                          {t('adminDashboard.cfgCount')}
                                        </th>
                                        <th scope="col">
                                          {t('adminDashboard.cfgFlags')}
                                        </th>
                                      </tr>
                                    </thead>
                                    <tbody>
                                      {selectedInventoryEntries.map(
                                        (entry, index) => {
                                          const baseId =
                                            typeof entry.baseId === 'number'
                                              ? entry.baseId
                                              : 0;
                                          const itemName = baseId
                                            ? catalogNameByBaseId.get(baseId) ??
                                              '-'
                                            : '-';
                                          const flags = [
                                            entry.worn
                                              ? t('adminDashboard.cfgWorn')
                                              : '',
                                            entry.wornLeft
                                              ? t('adminDashboard.cfgWornLeft')
                                              : '',
                                          ]
                                            .filter(Boolean)
                                            .join(', ');

                                          return (
                                            <tr key={`${baseId}-${index}`}>
                                              <td>
                                                {baseId
                                                  ? toCodeHex(baseId)
                                                  : '-'}
                                              </td>
                                              <td>{itemName}</td>
                                              <td>
                                                {typeof entry.count === 'number'
                                                  ? entry.count
                                                  : '-'}
                                              </td>
                                              <td>{flags || '-'}</td>
                                            </tr>
                                          );
                                        },
                                      )}
                                    </tbody>
                                  </table>
                                </div>
                              )}
                            </>
                          )}
                      </div>
                    )}

                    {statusMsg && (
                      <div
                        className="admin-dashboard__status-msg"
                        data-testid="admin-status-msg"
                        role="status"
                        aria-live="polite"
                      >
                        {statusMsg}
                      </div>
                    )}
                  </div>
                )}

                {activeMenuSurface === 'sidebar' &&
                  activeTab === 'players' &&
                  sendMsgTargetId !== null && (
                    <div
                      className="admin-dashboard__msg-form"
                      data-testid="admin-message-form"
                    >
                      <h3 className="admin-dashboard__section-subtitle">
                        {t('adminDashboard.sendMessageTitle')}:{' '}
                        {sendMsgTargetName}
                      </h3>
                      <div className="admin-dashboard__msg-row">
                        <input
                          className="admin-dashboard__msg-input"
                          data-testid="admin-message-input"
                          type="text"
                          placeholder={t('adminDashboard.messagePlaceholder')}
                          aria-label={t('adminDashboard.messagePlaceholder')}
                          value={sendMsgText}
                          onChange={(e) => setSendMsgText(e.target.value)}
                          onKeyDown={(e) => {
                            if (e.key === 'Enter') void sendMessageToPlayer();
                            if (e.key === 'Escape') {
                              setSendMsgTargetId(null);
                              setSendMsgText('');
                            }
                          }}
                          disabled={sendMsgSending}
                          autoFocus
                        />
                        <button
                          type="button"
                          className="admin-dashboard__msg-send-btn"
                          data-testid="admin-message-send-btn"
                          onClick={() => void sendMessageToPlayer()}
                          disabled={sendMsgSending || !sendMsgText.trim()}
                        >
                          {t('adminDashboard.messageSend')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__msg-cancel-btn"
                          data-testid="admin-message-cancel-btn"
                          onClick={() => {
                            setSendMsgTargetId(null);
                            setSendMsgText('');
                          }}
                        >
                          {t('adminDashboard.messageCancel')}
                        </button>
                      </div>
                    </div>
                  )}
                {activeMenuSurface === 'sidebar' && activeTab === 'console' && (
                  <div
                    className="admin-dashboard__panel"
                    id="admin-panel-console"
                    role="tabpanel"
                    aria-labelledby="admin-tab-console"
                  >
                    <div className="admin-dashboard__console-filter-row">
                      <input
                        className="admin-dashboard__search-input"
                        type="text"
                        placeholder={t(
                          'adminDashboard.consoleSearchPlaceholder',
                          { defaultValue: 'Search console logs…' },
                        )}
                        aria-label={t(
                          'adminDashboard.consoleSearchPlaceholder',
                          { defaultValue: 'Search console logs…' },
                        )}
                        value={consoleSearch}
                        onChange={(e) => setConsoleSearch(e.target.value)}
                      />
                      <button
                        type="button"
                        className="admin-dashboard__console-clear-btn"
                        onClick={() => setConsoleSearch('')}
                        disabled={consoleSearch.trim().length === 0}
                      >
                        {t('adminDashboard.clearSearch', {
                          defaultValue: 'Clear search',
                        })}
                      </button>
                    </div>

                    <div
                      className="admin-dashboard__console-out"
                      role="log"
                      aria-live="polite"
                      aria-relevant="additions text"
                    >
                      {filteredServerConsoleEntries.length === 0 &&
                        consoleLines.length === 0 && (
                          <div className="admin-dashboard__console-line admin-dashboard__console-line--input">
                            {t('adminDashboard.noLogs')}
                          </div>
                        )}
                      {filteredServerConsoleEntries.map((entry, i) => (
                        <div
                          key={`server-${entry.ts}-${i}`}
                          className={`admin-dashboard__console-line admin-dashboard__console-line--${
                            entry.level === 'error' ? 'err' : 'ok'
                          }`}
                        >
                          [{formatAdminTime(entry.ts)}] {entry.level ?? 'info'}:{' '}
                          {entry.message}
                        </div>
                      ))}
                      {consoleLines.length > 0 && (
                        <div className="admin-dashboard__console-line admin-dashboard__console-line--input">
                          {t('adminDashboard.consoleCommandOutput')}
                        </div>
                      )}
                      {consoleLines.map((line, i) => (
                        <div
                          key={i}
                          className={`admin-dashboard__console-line admin-dashboard__console-line--${line.kind}`}
                        >
                          {line.text}
                        </div>
                      ))}
                      <div ref={consoleEndRef} />
                    </div>

                    <div className="admin-dashboard__console-row">
                      <input
                        className="admin-dashboard__console-input"
                        type="text"
                        placeholder={t('adminDashboard.consoleHint')}
                        aria-label={t('adminDashboard.consoleHint')}
                        value={consoleInput}
                        onChange={(e) => setConsoleInput(e.target.value)}
                        onKeyDown={handleConsoleKeyDown}
                        disabled={consoleSending || !capabilities.canConsole}
                      />
                      <button
                        type="button"
                        className="admin-dashboard__console-send-btn"
                        onClick={() => void sendConsoleCommand()}
                        disabled={consoleSending || !capabilities.canConsole}
                      >
                        {t('adminDashboard.consoleSend')}
                      </button>
                    </div>

                    <div className="admin-dashboard__console-history">
                      <span className="admin-dashboard__console-history-label">
                        {t('adminDashboard.consoleHistory')}
                      </span>
                      {consoleSearch.trim().length > 0 && (
                        <span className="admin-dashboard__console-history-empty">
                          {t('adminDashboard.consoleFilteredCount', {
                            defaultValue:
                              '{{shown}} of {{total}} entries visible',
                            shown: filteredServerConsoleEntries.length,
                            total: serverConsoleEntries.length,
                          })}
                        </span>
                      )}
                      <button
                        type="button"
                        className="admin-dashboard__console-clear-btn"
                        onClick={() => setConsoleLines([])}
                        disabled={consoleLines.length === 0}
                      >
                        {t('adminDashboard.consoleClearOutput')}
                      </button>
                      <div className="admin-dashboard__console-history-list">
                        {consoleHistory.length === 0 ? (
                          <span className="admin-dashboard__console-history-empty">
                            {t('adminDashboard.consoleHistoryEmpty')}
                          </span>
                        ) : (
                          consoleHistory.slice(0, 8).map((item) => (
                            <button
                              key={item}
                              type="button"
                              className="admin-dashboard__console-history-item"
                              onClick={() => setConsoleInput(item)}
                              disabled={!capabilities.canConsole}
                            >
                              {item}
                            </button>
                          ))
                        )}
                      </div>
                      <button
                        type="button"
                        className="admin-dashboard__console-clear-btn"
                        onClick={() => {
                          setConsoleHistory([]);
                          setConsoleHistoryIndex(null);
                        }}
                        disabled={consoleHistory.length === 0}
                      >
                        {t('adminDashboard.consoleClearHistory')}
                      </button>
                    </div>

                    {!capabilities.canConsole && (
                      <p className="admin-dashboard__console-note admin-dashboard__console-note--warn">
                        {t('adminDashboard.consoleDisabled')}
                      </p>
                    )}
                    <p className="admin-dashboard__console-note">
                      {t('adminDashboard.consoleNote')}
                    </p>
                  </div>
                )}

                {activeMenuSurface === 'sidebar' && activeTab === 'logs' && (
                  <div
                    className="admin-dashboard__panel"
                    id="admin-panel-logs"
                    role="tabpanel"
                    aria-labelledby="admin-tab-logs"
                  >
                    {!capabilities.canViewLogs && (
                      <p className="admin-dashboard__no-players">
                        {t('adminDashboard.logsDisabled')}
                      </p>
                    )}

                    {capabilities.canViewLogs && (
                      <>
                        <div className="admin-dashboard__panel-toolbar">
                          <div className="admin-dashboard__panel-toolbar-left">
                            <span className="admin-dashboard__panel-toolbar-title">
                              {t('adminDashboard.tabLogs')}
                            </span>
                            {filteredLogEntries.length > 0 && (
                              <span className="admin-dashboard__panel-badge">
                                {filteredLogEntries.length}
                              </span>
                            )}
                          </div>
                          <div className="admin-dashboard__panel-toolbar-right">
                            <button
                              type="button"
                              className="admin-dashboard__log-page-btn"
                              onClick={openRecentLogs}
                              disabled={logBeforeTs === null}
                            >
                              {t('adminDashboard.logRecent')}
                            </button>
                            <button
                              type="button"
                              className="admin-dashboard__log-page-btn"
                              onClick={openOlderLogs}
                              disabled={!logHasMore || oldestLogTs === null}
                            >
                              {t('adminDashboard.logOlder')}
                            </button>
                            <button
                              type="button"
                              className="admin-dashboard__log-page-btn"
                              onClick={() => {
                                setLogTypeFilter('');
                                setLogLevelFilter('');
                                setLogSinceMinutes('');
                                setLogTextFilter('');
                                setLogBeforeTs(null);
                              }}
                              disabled={
                                !hasActiveLogFilters && logBeforeTs === null
                              }
                            >
                              {t('adminDashboard.clearFilters', {
                                defaultValue: 'Clear filters',
                              })}
                            </button>
                          </div>
                        </div>
                        <div className="admin-dashboard__log-tools">
                          <input
                            className="admin-dashboard__search-input admin-dashboard__search-input--reason"
                            type="text"
                            placeholder={t(
                              'adminDashboard.logSearchPlaceholder',
                              { defaultValue: 'Search logs…' },
                            )}
                            aria-label={t(
                              'adminDashboard.logSearchPlaceholder',
                              { defaultValue: 'Search logs…' },
                            )}
                            value={logTextFilter}
                            onChange={(e) => setLogTextFilter(e.target.value)}
                          />

                          <label className="admin-dashboard__log-tool">
                            <span>{t('adminDashboard.logLimit')}</span>
                            <select
                              className="admin-dashboard__log-select"
                              value={logLimit}
                              aria-label={t('adminDashboard.logLimit')}
                              onChange={(e) =>
                                setLogLimit(Number(e.target.value))
                              }
                            >
                              {[25, 50, 100, 200].map((value) => (
                                <option key={value} value={value}>
                                  {value}
                                </option>
                              ))}
                            </select>
                          </label>

                          <label className="admin-dashboard__log-tool">
                            <span>{t('adminDashboard.logWindow')}</span>
                            <select
                              className="admin-dashboard__log-select"
                              value={logSinceMinutes}
                              aria-label={t('adminDashboard.logWindow')}
                              onChange={(e) =>
                                setLogSinceMinutes(
                                  e.target.value as '' | '15' | '60' | '1440',
                                )
                              }
                            >
                              <option value="">
                                {t('adminDashboard.logWindowAll')}
                              </option>
                              <option value="15">
                                {t('adminDashboard.logWindow15m')}
                              </option>
                              <option value="60">
                                {t('adminDashboard.logWindow1h')}
                              </option>
                              <option value="1440">
                                {t('adminDashboard.logWindow24h')}
                              </option>
                            </select>
                          </label>

                          <label className="admin-dashboard__log-tool">
                            <span>{t('adminDashboard.logLevel')}</span>
                            <select
                              className="admin-dashboard__log-select"
                              value={logLevelFilter}
                              aria-label={t('adminDashboard.logLevel')}
                              onChange={(e) =>
                                setLogLevelFilter(
                                  e.target.value as '' | ServerLogLevel,
                                )
                              }
                            >
                              <option value="">
                                {t('adminDashboard.logLevelAll')}
                              </option>
                              <option value="info">
                                {t('adminDashboard.logLevel_info')}
                              </option>
                              <option value="error">
                                {t('adminDashboard.logLevel_error')}
                              </option>
                            </select>
                          </label>
                        </div>

                        <div className="admin-dashboard__log-filters">
                          {(
                            [
                              '',
                              'server',
                              'kick',
                              'ban',
                              'mute',
                              'console',
                            ] as const
                          ).map((type) => (
                            <button
                              key={type}
                              type="button"
                              className={`admin-dashboard__log-filter${
                                logTypeFilter === type
                                  ? ' admin-dashboard__log-filter--active'
                                  : ''
                              }`}
                              onClick={() => setLogTypeFilter(type)}
                              aria-pressed={logTypeFilter === type}
                            >
                              {type === ''
                                ? t('adminDashboard.logAll')
                                : t(`adminDashboard.logType_${type}`)}
                            </button>
                          ))}
                        </div>

                        {hasActiveLogFilters && (
                          <div className="admin-dashboard__log-active-filters">
                            {t('adminDashboard.activeFiltersHint', {
                              defaultValue:
                                'Active filters are affecting the results.',
                            })}
                          </div>
                        )}

                        {filteredLogEntries.length === 0 ? (
                          <p className="admin-dashboard__no-players">
                            {t('adminDashboard.noLogs')}
                          </p>
                        ) : (
                          <div className="admin-dashboard__log-list">
                            {filteredLogEntries.map((entry, i) => (
                              <div
                                key={i}
                                className="admin-dashboard__log-entry"
                              >
                                <span className="admin-dashboard__log-ts">
                                  {formatAdminTime(entry.ts)}
                                </span>
                                <span
                                  className={`admin-dashboard__log-type admin-dashboard__log-type--${entry.type}`}
                                >
                                  {entry.type}
                                </span>
                                {entry.level && (
                                  <span
                                    className={`admin-dashboard__log-type admin-dashboard__log-type--${
                                      entry.level === 'error'
                                        ? 'ban'
                                        : 'console'
                                    }`}
                                  >
                                    {entry.level}
                                  </span>
                                )}
                                <span className="admin-dashboard__log-msg">
                                  {entry.message}
                                </span>
                                {capabilities.canViewLogs &&
                                  extractHistoryUserIdFromLog(entry) && (
                                    <button
                                      type="button"
                                      style={{ marginLeft: 'auto' }}
                                      onClick={() =>
                                        openTopbarHistorySearch({
                                          search: extractHistoryUserIdFromLog(
                                            entry,
                                          ),
                                          mode: 'player',
                                          actionType:
                                            inferHistoryActionTypeFromLog(entry),
                                        })
                                      }
                                    >
                                      {t('adminDashboard.topTx_history', {
                                        defaultValue: 'History',
                                      })}
                                    </button>
                                  )}
                              </div>
                            ))}
                          </div>
                        )}
                      </>
                    )}
                  </div>
                )}

                {activeMenuSurface === 'sidebar' && activeTab === 'metrics' && (
                  <div
                    className="admin-dashboard__panel"
                    id="admin-panel-metrics"
                    role="tabpanel"
                    aria-labelledby="admin-tab-metrics"
                  >
                    {!capabilities.canViewLogs && (
                      <p className="admin-dashboard__no-players">
                        {t('adminDashboard.metricsDisabled')}
                      </p>
                    )}

                    {capabilities.canViewLogs && (
                      <>
                        <div className="admin-dashboard__metrics-toolbar">
                          <input
                            className="admin-dashboard__metrics-input"
                            type="text"
                            placeholder={t(
                              'adminDashboard.metricsSearchPlaceholder',
                            )}
                            aria-label={t(
                              'adminDashboard.metricsSearchPlaceholder',
                            )}
                            value={metricNameFilter}
                            onChange={(e) =>
                              setMetricNameFilter(e.target.value)
                            }
                          />

                          <select
                            className="admin-dashboard__log-select"
                            value={metricSourceFilter}
                            aria-label={t('adminDashboard.metricsAllSources')}
                            onChange={(e) =>
                              setMetricSourceFilter(e.target.value)
                            }
                          >
                            <option value="">
                              {t('adminDashboard.metricsAllSources')}
                            </option>
                            {metricSourceOptions.map((source) => (
                              <option key={source} value={source}>
                                {source}
                              </option>
                            ))}
                          </select>

                          <select
                            className="admin-dashboard__log-select"
                            value={metricLimit}
                            aria-label={t('adminDashboard.logLimit')}
                            onChange={(e) =>
                              setMetricLimit(Number(e.target.value))
                            }
                          >
                            {[25, 50, 100, 200].map((value) => (
                              <option key={value} value={value}>
                                {value}
                              </option>
                            ))}
                          </select>

                          <button
                            type="button"
                            className="admin-dashboard__log-page-btn"
                            onClick={() => void fetchFrontendMetrics()}
                          >
                            {t('adminDashboard.metricsRefresh')}
                          </button>
                        </div>

                        <div className="admin-dashboard__metrics-grid">
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.metricsTotal')}
                            </span>
                            <span className="admin-dashboard__metric-value">
                              {metricSummary.totalCount}
                            </span>
                          </div>
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.metricsErrors')}
                            </span>
                            <span className="admin-dashboard__metric-value admin-dashboard__metric-value--warn">
                              {metricSummary.errorCount}
                            </span>
                          </div>
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.metricsAverage')}
                            </span>
                            <span className="admin-dashboard__metric-value">
                              {metricSummary.averageValue}
                            </span>
                          </div>
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.metricsLastReceived')}
                            </span>
                            <span className="admin-dashboard__metric-value admin-dashboard__metric-value--small">
                              {metricSummary.lastReceivedAt
                                ? formatAdminTime(metricSummary.lastReceivedAt)
                                : '-'}
                            </span>
                          </div>
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.wsHealthTitle', {
                                defaultValue: 'WebSocket updates health',
                              })}
                            </span>
                            <span
                              className={`admin-dashboard__metric-value admin-dashboard__metric-value--${wsHealthTone}`}
                            >
                              {wsHealthLabel}
                            </span>
                          </div>
                          <div className="admin-dashboard__metric-card">
                            <span className="admin-dashboard__metric-label">
                              {t('adminDashboard.wsMessagesTotal', {
                                defaultValue: 'WS messages (session)',
                              })}
                            </span>
                            <span className="admin-dashboard__metric-value">
                              {wsTotalMessages}
                            </span>
                            <span className="admin-dashboard__metric-subvalue">
                              {t('adminDashboard.wsReconnects', {
                                defaultValue: 'Reconnects',
                              })}
                              : {wsReconnectCount}
                            </span>
                            <span className="admin-dashboard__metric-subvalue">
                              {t('adminDashboard.wsLastMessage', {
                                defaultValue: 'Last WS message',
                              })}
                              : {wsLastMessageLabel}
                            </span>
                            <div className="admin-dashboard__metric-sublist">
                              {wsReconnectEvents.length === 0 && (
                                <span className="admin-dashboard__metric-subvalue">
                                  {t('adminDashboard.wsEventsEmpty', {
                                    defaultValue: 'No reconnect events yet.',
                                  })}
                                </span>
                              )}
                              {wsReconnectEvents.map((event, index) => (
                                <span
                                  key={`${event.ts}-${event.kind}-${index}`}
                                  className={`admin-dashboard__metric-subvalue admin-dashboard__metric-subvalue--${event.kind}`}
                                >
                                  {formatAdminTime(event.ts)}: {' '}
                                  {event.kind === 'reconnected'
                                    ? t('adminDashboard.wsEvent_reconnected', {
                                        defaultValue: 'Reconnected',
                                      })
                                    : event.kind === 'reconnecting'
                                      ? t('adminDashboard.wsEvent_reconnecting', {
                                          defaultValue: 'Reconnecting',
                                        })
                                      : t('adminDashboard.wsEvent_fallback', {
                                          defaultValue: 'HTTP fallback',
                                        })}
                                </span>
                              ))}
                            </div>
                          </div>
                        </div>

                        <div className="admin-dashboard__metrics-groups">
                          <div className="admin-dashboard__bans-section">
                            <h3 className="admin-dashboard__section-subtitle">
                              {t('adminDashboard.metricsTopSources')}
                            </h3>
                            <div className="admin-dashboard__metric-chips">
                              {metricSummary.sources.length === 0 && (
                                <span className="admin-dashboard__console-history-empty">
                                  -
                                </span>
                              )}
                              {metricSummary.sources.map((item) => (
                                <span
                                  key={item.name}
                                  className="admin-dashboard__metric-chip"
                                >
                                  {item.name} ({item.count})
                                </span>
                              ))}
                            </div>
                          </div>

                          <div className="admin-dashboard__bans-section">
                            <h3 className="admin-dashboard__section-subtitle">
                              {t('adminDashboard.metricsTopNames')}
                            </h3>
                            <div className="admin-dashboard__metric-chips">
                              {metricSummary.names.length === 0 && (
                                <span className="admin-dashboard__console-history-empty">
                                  -
                                </span>
                              )}
                              {metricSummary.names.map((item) => (
                                <span
                                  key={item.name}
                                  className="admin-dashboard__metric-chip"
                                >
                                  {item.name} ({item.count})
                                </span>
                              ))}
                            </div>
                          </div>
                        </div>

                        {metricEntries.length === 0 ? (
                          <p className="admin-dashboard__no-players">
                            {t('adminDashboard.metricsEmpty')}
                          </p>
                        ) : (
                          <div className="admin-dashboard__metrics-list">
                            {metricEntries.map((entry, index) => (
                              <div
                                key={`${entry.name}-${entry.receivedAt}-${index}`}
                                className="admin-dashboard__metrics-entry"
                              >
                                <div className="admin-dashboard__metrics-main">
                                  <span className="admin-dashboard__metrics-name">
                                    {entry.name}
                                  </span>
                                  <span className="admin-dashboard__metrics-source">
                                    {entry.source}
                                  </span>
                                  <span className="admin-dashboard__metrics-value">
                                    {entry.value}
                                  </span>
                                </div>
                                <div className="admin-dashboard__metrics-meta">
                                  <span>
                                    {t('adminDashboard.metricsEventTs')}:{' '}
                                    {formatAdminTime(entry.ts)}
                                  </span>
                                  <span>
                                    {t('adminDashboard.metricsReceivedTs')}:{' '}
                                    {formatAdminTime(entry.receivedAt)}
                                  </span>
                                  {entry.clientSource && (
                                    <span>Client: {entry.clientSource}</span>
                                  )}
                                  {entry.sessionId && (
                                    <span>Session: {entry.sessionId}</span>
                                  )}
                                  {entry.path && (
                                    <span>Path: {entry.path}</span>
                                  )}
                                  {entry.visibilityState && (
                                    <span>
                                      Visibility: {entry.visibilityState}
                                    </span>
                                  )}
                                  {entry.language && (
                                    <span>Lang: {entry.language}</span>
                                  )}
                                  {entry.platform && (
                                    <span>Platform: {entry.platform}</span>
                                  )}
                                  {entry.userAgent && (
                                    <span>UA: {entry.userAgent}</span>
                                  )}
                                  {entry.url && <span>{entry.url}</span>}
                                </div>
                              </div>
                            ))}
                          </div>
                        )}

                        <div
                          className="admin-dashboard__metrics-groups"
                          style={{ marginTop: 16 }}
                        >
                          <div className="admin-dashboard__bans-section">
                            <h3 className="admin-dashboard__section-subtitle">
                              Client Runtime Events (skymp5-client)
                            </h3>
                            <div className="admin-dashboard__metric-chips">
                              <span className="admin-dashboard__metric-chip">
                                Total {clientRuntimeSummary.totalCount}
                              </span>
                              <span className="admin-dashboard__metric-chip">
                                Errors {clientRuntimeSummary.errorCount}
                              </span>
                              <span className="admin-dashboard__metric-chip">
                                Warnings {clientRuntimeSummary.warnCount}
                              </span>
                              <span className="admin-dashboard__metric-chip">
                                Last{' '}
                                {clientRuntimeSummary.lastReceivedAt
                                  ? formatAdminTime(
                                      clientRuntimeSummary.lastReceivedAt,
                                    )
                                  : '-'}
                              </span>
                            </div>
                          </div>
                        </div>

                        {clientRuntimeEntries.length === 0 ? (
                          <p className="admin-dashboard__no-players">
                            No client runtime events yet.
                          </p>
                        ) : (
                          <div
                            className="admin-dashboard__log-list"
                            style={{ marginTop: 10 }}
                          >
                            {clientRuntimeEntries.map((entry, index) => (
                              <div
                                key={`${entry.userId}-${entry.receivedAt}-${index}`}
                                className="admin-dashboard__log-entry"
                              >
                                <span className="admin-dashboard__log-ts">
                                  {formatAdminTime(entry.receivedAt)}
                                </span>
                                <span
                                  className={`admin-dashboard__log-type admin-dashboard__log-type--${
                                    entry.level === 'error'
                                      ? 'ban'
                                      : entry.level === 'warn'
                                      ? 'kick'
                                      : 'console'
                                  }`}
                                >
                                  {entry.level}
                                </span>
                                <span className="admin-dashboard__log-msg">
                                  [{entry.event}] userId={entry.userId}{' '}
                                  {entry.ip ? `ip=${entry.ip}` : ''}{' '}
                                  {entry.details || ''}
                                </span>
                                {capabilities.canViewLogs && (
                                  <button
                                    type="button"
                                    style={{ marginLeft: 'auto' }}
                                    onClick={() =>
                                      openTopbarHistorySearch({
                                        search: String(entry.userId),
                                        mode: 'player',
                                      })
                                    }
                                  >
                                    {t('adminDashboard.topTx_history', {
                                      defaultValue: 'History',
                                    })}
                                  </button>
                                )}
                              </div>
                            ))}
                          </div>
                        )}
                      </>
                    )}
                  </div>
                )}

                {activeMenuSurface === 'sidebar' && activeTab === 'cfg' && (
                  <div
                    className="admin-dashboard__panel"
                    id="admin-panel-cfg"
                    role="tabpanel"
                    aria-labelledby="admin-tab-cfg"
                  >
                    <div className="admin-dashboard__panel-toolbar">
                      <div className="admin-dashboard__panel-toolbar-left">
                        <span className="admin-dashboard__panel-toolbar-title">
                          {t('adminDashboard.cfgEditorTitle')}
                        </span>
                      </div>
                      <div className="admin-dashboard__panel-toolbar-right">
                        <button
                          type="button"
                          className="admin-dashboard__log-btn"
                          onClick={() => void loadCfgEditor()}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {t('adminDashboard.cfgLoad')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn"
                          onClick={() => {
                            try {
                              const normalized = mergeCfgFormIntoJson(
                                cfgEditorText || '{}',
                                cfgForm,
                              );
                              setCfgEditorText(normalized);
                              setCfgEditorStatus(
                                t('adminDashboard.cfgFormApplied'),
                              );
                            } catch {
                              setCfgEditorStatus(
                                t('adminDashboard.cfgInvalidJson'),
                              );
                            }
                          }}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {t('adminDashboard.cfgApplyForm')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn"
                          onClick={formatCfgEditor}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {t('adminDashboard.cfgFormat')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn"
                          onClick={applyAccessDiscordToCfgEditor}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {t('adminDashboard.cfgApplyAccess')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn admin-dashboard__log-btn--primary"
                          onClick={async () => {
                            const normalized = applyAccessDiscordToCfgEditor();
                            if (!normalized) return;
                            await saveCfgEditor(normalized);
                          }}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {t('adminDashboard.cfgSaveAccess')}
                        </button>
                        <button
                          type="button"
                          className="admin-dashboard__log-btn admin-dashboard__log-btn--primary"
                          onClick={() => void saveCfgEditor()}
                          disabled={cfgEditorLoading || cfgEditorSaving}
                        >
                          {cfgEditorSaving
                            ? t('adminDashboard.loading')
                            : t('adminDashboard.cfgSave')}
                        </button>
                      </div>
                    </div>

                    <div
                      className="admin-dashboard__cfg-tabs"
                      role="tablist"
                      aria-label={t('adminDashboard.cfgEditorTitle')}
                    >
                      {(
                        [
                          'general',
                          'access',
                          'inventory',
                          'json',
                        ] as CfgEditorTab[]
                      ).map((tab) => (
                        <button
                          key={tab}
                          type="button"
                          role="tab"
                          aria-selected={cfgEditorTab === tab}
                          className={`admin-dashboard__cfg-tab${
                            cfgEditorTab === tab
                              ? ' admin-dashboard__cfg-tab--active'
                              : ''
                          }`}
                          onClick={() => setCfgEditorTab(tab)}
                        >
                          {t(`adminDashboard.cfgTab_${tab}`)}
                        </button>
                      ))}
                    </div>

                    {cfgEditorTab === 'general' && (
                      <div className="admin-dashboard__cfg-form-grid">
                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgServerName')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.serverName}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                serverName: e.target.value,
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgMasterUrl')}</span>
                          <select
                            className="admin-dashboard__log-select"
                            value={cfgForm.masterUrl}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                masterUrl: e.target.value,
                              }))
                            }
                          >
                            {MASTER_URL_OPTIONS.map((url) => (
                              <option key={url} value={url}>
                                {url}
                              </option>
                            ))}
                          </select>
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgMasterKey')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.masterKey}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                masterKey: e.target.value,
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgPort')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="number"
                            min={1}
                            value={cfgForm.port}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                port: Number(e.target.value) || 1,
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgMaxPlayers')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="number"
                            min={1}
                            value={cfgForm.maxPlayers}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                maxPlayers: Number(e.target.value) || 1,
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgLanguage')}</span>
                          <select
                            className="admin-dashboard__log-select"
                            value={cfgForm.defaultLanguage}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                defaultLanguage: e.target.value,
                              }))
                            }
                          >
                            <option value="en">en</option>
                            <option value="de">de</option>
                            <option value="es">es</option>
                            <option value="fr">fr</option>
                            <option value="it">it</option>
                            <option value="ru">ru</option>
                          </select>
                        </label>

                        <label className="admin-dashboard__cfg-check">
                          <input
                            type="checkbox"
                            checked={cfgForm.offlineMode}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                offlineMode: e.target.checked,
                              }))
                            }
                          />
                          <span>{t('adminDashboard.cfgOfflineMode')}</span>
                        </label>

                        <label className="admin-dashboard__cfg-check">
                          <input
                            type="checkbox"
                            checked={cfgForm.showCharacterNicknames}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                showCharacterNicknames: e.target.checked,
                              }))
                            }
                          />
                          <span>{t('adminDashboard.cfgShowCharacterNicknames')}</span>
                        </label>

                        <div className="admin-dashboard__cfg-section-title">
                          {t('adminDashboard.cfgSupervisorTitle')}
                        </div>
                        <p className="admin-dashboard__cfg-hint">
                          {t('adminDashboard.cfgSupervisorHint')}
                        </p>
                        <label className="admin-dashboard__cfg-check">
                          <input
                            type="checkbox"
                            checked={cfgForm.supervisorEnabled}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                supervisorEnabled: e.target.checked,
                              }))
                            }
                          />
                          <span>{t('adminDashboard.cfgSupervisorEnabled')}</span>
                        </label>
                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgSupervisorStopCommand')}</span>
                          <input
                            type="text"
                            className="admin-dashboard__search-input"
                            placeholder={t('adminDashboard.cfgSupervisorStopCommandPlaceholder')}
                            value={cfgForm.supervisorStopCommand}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                supervisorStopCommand: e.target.value,
                              }))
                            }
                          />
                        </label>
                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgSupervisorRestartCommand')}</span>
                          <input
                            type="text"
                            className="admin-dashboard__search-input"
                            placeholder={t('adminDashboard.cfgSupervisorRestartCommandPlaceholder')}
                            value={cfgForm.supervisorRestartCommand}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                supervisorRestartCommand: e.target.value,
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgItemPickupMode')}</span>
                          <select
                            className="admin-dashboard__log-select"
                            value={cfgForm.itemPickupMode}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                itemPickupMode: e.target
                                  .value as CfgFormState['itemPickupMode'],
                              }))
                            }
                          >
                            <option value="allow">
                              {t('adminDashboard.cfgItemPickupMode_allow')}
                            </option>
                            <option value="minigame">
                              {t('adminDashboard.cfgItemPickupMode_minigame')}
                            </option>
                            <option value="block-all">
                              {t('adminDashboard.cfgItemPickupMode_blockAll')}
                            </option>
                          </select>
                        </label>
                      </div>
                    )}

                    {cfgEditorTab === 'access' && (
                      <div className="admin-dashboard__cfg-form-grid">
                        <label className="admin-dashboard__cfg-check">
                          <input
                            type="checkbox"
                            checked={cfgForm.discordBot.enabled}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                discordBot: {
                                  ...prev.discordBot,
                                  enabled: e.target.checked,
                                },
                              }))
                            }
                          />
                          <span>{t('adminDashboard.cfgDiscordEnabled')}</span>
                        </label>

                        <label className="admin-dashboard__cfg-field admin-dashboard__cfg-field--full">
                          <span>{t('adminDashboard.cfgJoinMode')}</span>
                          <select
                            className="admin-dashboard__log-select"
                            value={cfgForm.joinAccess.mode}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                joinAccess: {
                                  ...prev.joinAccess,
                                  mode: e.target
                                    .value as JoinAccessForm['mode'],
                                },
                              }))
                            }
                          >
                            <option value="none">
                              {t('adminDashboard.cfgJoinMode_none')}
                            </option>
                            <option value="approvedLicense">
                              {t('adminDashboard.cfgJoinMode_approvedLicense')}
                            </option>
                            <option value="discordMember">
                              {t('adminDashboard.cfgJoinMode_discordMember')}
                            </option>
                            <option value="discordRoles">
                              {t('adminDashboard.cfgJoinMode_discordRoles')}
                            </option>
                          </select>
                        </label>

                        <label className="admin-dashboard__cfg-field admin-dashboard__cfg-field--full">
                          <span>{t('adminDashboard.cfgRejectMessage')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.joinAccess.rejectionMessage}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                joinAccess: {
                                  ...prev.joinAccess,
                                  rejectionMessage: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgApprovedLicenses')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.joinAccess.approvedLicenses}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                joinAccess: {
                                  ...prev.joinAccess,
                                  approvedLicenses: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>
                            {t('adminDashboard.cfgApprovedDiscordIds')}
                          </span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.joinAccess.approvedDiscordIds}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                joinAccess: {
                                  ...prev.joinAccess,
                                  approvedDiscordIds: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgDiscordRoles')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.joinAccess.discordRoleIds}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                joinAccess: {
                                  ...prev.joinAccess,
                                  discordRoleIds: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgDiscordToken')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="password"
                            value={cfgForm.discordBot.token}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                discordBot: {
                                  ...prev.discordBot,
                                  token: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>{t('adminDashboard.cfgDiscordGuildId')}</span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.discordBot.guildId}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                discordBot: {
                                  ...prev.discordBot,
                                  guildId: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>

                        <label className="admin-dashboard__cfg-field">
                          <span>
                            {t('adminDashboard.cfgDiscordWarningsChannel')}
                          </span>
                          <input
                            className="admin-dashboard__search-input"
                            type="text"
                            value={cfgForm.discordBot.warningsChannelId}
                            onChange={(e) =>
                              setCfgForm((prev) => ({
                                ...prev,
                                discordBot: {
                                  ...prev.discordBot,
                                  warningsChannelId: e.target.value,
                                },
                              }))
                            }
                          />
                        </label>
                      </div>
                    )}

                    {cfgEditorTab === 'inventory' && (
                      <>
                        <div className="admin-dashboard__cfg-inventory">
                          <h4 className="admin-dashboard__section-subtitle">
                            {t('adminDashboard.cfgStartSetup')}
                          </h4>
                          <div className="admin-dashboard__cfg-form-grid">
                            <label className="admin-dashboard__cfg-field">
                              <span>{t('adminDashboard.cfgSpawnPreset')}</span>
                              <select
                                className="admin-dashboard__log-select"
                                value={selectedSpawnPresetKey}
                                onChange={(e) => {
                                  const preset = SPAWN_PRESETS.find(
                                    (entry) => entry.key === e.target.value,
                                  );
                                  if (!preset) {
                                    return;
                                  }
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      x: preset.x,
                                      y: preset.y,
                                      z: preset.z,
                                      worldOrCell: preset.worldOrCell,
                                      angleZ: preset.angleZ,
                                    },
                                  }));
                                }}
                              >
                                <option value="">
                                  {t('adminDashboard.cfgSpawnPreset_custom')}
                                </option>
                                {SPAWN_PRESETS.map((preset) => (
                                  <option key={preset.key} value={preset.key}>
                                    {t(`adminDashboard.${preset.labelKey}`)}
                                  </option>
                                ))}
                              </select>
                            </label>

                            <label className="admin-dashboard__cfg-field">
                              <span>{t('adminDashboard.cfgSpawnX')}</span>
                              <input
                                className="admin-dashboard__search-input"
                                type="number"
                                value={cfgForm.startSpawn.x}
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      ...prev.startSpawn,
                                      x: Number(e.target.value) || 0,
                                    },
                                  }))
                                }
                              />
                            </label>

                            <label className="admin-dashboard__cfg-field">
                              <span>{t('adminDashboard.cfgSpawnY')}</span>
                              <input
                                className="admin-dashboard__search-input"
                                type="number"
                                value={cfgForm.startSpawn.y}
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      ...prev.startSpawn,
                                      y: Number(e.target.value) || 0,
                                    },
                                  }))
                                }
                              />
                            </label>

                            <label className="admin-dashboard__cfg-field">
                              <span>{t('adminDashboard.cfgSpawnZ')}</span>
                              <input
                                className="admin-dashboard__search-input"
                                type="number"
                                value={cfgForm.startSpawn.z}
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      ...prev.startSpawn,
                                      z: Number(e.target.value) || 0,
                                    },
                                  }))
                                }
                              />
                            </label>

                            <label className="admin-dashboard__cfg-field">
                              <span>
                                {t('adminDashboard.cfgSpawnWorldOrCell')}
                              </span>
                              <select
                                className="admin-dashboard__log-select"
                                value={selectedWorldOrCellValue}
                                onChange={(e) => {
                                  const nextValue = e.target.value;
                                  if (nextValue === '__custom__') {
                                    return;
                                  }
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      ...prev.startSpawn,
                                      worldOrCell: nextValue,
                                    },
                                  }));
                                }}
                              >
                                {selectedWorldOrCellValue === '__custom__' && (
                                  <option value="__custom__">
                                    {t(
                                      'adminDashboard.cfgWorldOrCellOption_custom',
                                    )}
                                  </option>
                                )}
                                {WORLD_OR_CELL_OPTIONS.map((option) => (
                                  <option
                                    key={option.value}
                                    value={option.value}
                                  >
                                    {t(`adminDashboard.${option.labelKey}`)}
                                  </option>
                                ))}
                              </select>
                              <span className="admin-dashboard__cfg-hint">
                                {t(`adminDashboard.${worldOrCellHintKey}`)}
                              </span>
                            </label>

                            <label className="admin-dashboard__cfg-field">
                              <span>{t('adminDashboard.cfgSpawnAngleZ')}</span>
                              <input
                                className="admin-dashboard__search-input"
                                type="number"
                                value={cfgForm.startSpawn.angleZ}
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    startSpawn: {
                                      ...prev.startSpawn,
                                      angleZ: Number(e.target.value) || 0,
                                    },
                                  }))
                                }
                              />
                            </label>
                          </div>
                        </div>

                        <div className="admin-dashboard__cfg-inventory">
                          <h4 className="admin-dashboard__section-subtitle">
                            {t('adminDashboard.cfgNpcSettings')}
                          </h4>
                          <div className="admin-dashboard__cfg-form-grid">
                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={cfgForm.npcEnabled}
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    npcEnabled: e.target.checked,
                                  }))
                                }
                              />
                              <span>{t('adminDashboard.cfgNpcEnabled')}</span>
                            </label>

                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={
                                  cfgForm.npcDefaultSettings.spawnInInterior
                                }
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    npcDefaultSettings: {
                                      ...prev.npcDefaultSettings,
                                      spawnInInterior: e.target.checked,
                                    },
                                  }))
                                }
                              />
                              <span>
                                {t('adminDashboard.cfgNpcSpawnInInterior')}
                              </span>
                            </label>

                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={
                                  cfgForm.npcDefaultSettings.spawnInExterior
                                }
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    npcDefaultSettings: {
                                      ...prev.npcDefaultSettings,
                                      spawnInExterior: e.target.checked,
                                    },
                                  }))
                                }
                              />
                              <span>
                                {t('adminDashboard.cfgNpcSpawnInExterior')}
                              </span>
                            </label>

                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={
                                  cfgForm.npcDefaultSettings.allowHumanoid
                                }
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    npcDefaultSettings: {
                                      ...prev.npcDefaultSettings,
                                      allowHumanoid: e.target.checked,
                                    },
                                  }))
                                }
                              />
                              <span>
                                {t('adminDashboard.cfgNpcAllowHumanoid')}
                              </span>
                            </label>

                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={
                                  cfgForm.npcDefaultSettings.allowCreature
                                }
                                onChange={(e) =>
                                  setCfgForm((prev) => ({
                                    ...prev,
                                    npcDefaultSettings: {
                                      ...prev.npcDefaultSettings,
                                      allowCreature: e.target.checked,
                                    },
                                  }))
                                }
                              />
                              <span>
                                {t('adminDashboard.cfgNpcAllowCreature')}
                              </span>
                            </label>

                            <div className="admin-dashboard__cfg-field admin-dashboard__cfg-field--full">
                              <span className="admin-dashboard__cfg-hint">
                                {t('adminDashboard.cfgNpcHint')}
                              </span>
                            </div>
                          </div>
                        </div>

                        <div className="admin-dashboard__cfg-inventory">
                          <h4 className="admin-dashboard__section-subtitle">
                            {t('adminDashboard.cfgStarterInventory')}
                          </h4>
                          <div className="admin-dashboard__cfg-inventory-controls">
                            <select
                              className="admin-dashboard__log-select"
                              value={inventoryCategory}
                              onChange={(e) =>
                                setInventoryCategory(
                                  e.target.value as ItemCategory,
                                )
                              }
                            >
                              {ITEM_CATEGORIES.map((category) => (
                                <option key={category} value={category}>
                                  {t(`adminDashboard.itemCategory_${category}`)}
                                </option>
                              ))}
                            </select>

                            <select
                              className="admin-dashboard__log-select"
                              value={inventoryItemCode}
                              onChange={(e) =>
                                setInventoryItemCode(e.target.value)
                              }
                            >
                              {filteredCatalogItems.map((item) => (
                                <option
                                  key={item.codeHex}
                                  value={item.codeHex}
                                >{`${t(`items.${item.codeHex}`, {
                                  defaultValue: item.name,
                                })} (${item.codeHex})`}</option>
                              ))}
                            </select>

                            <input
                              className="admin-dashboard__search-input"
                              type="text"
                              value={inventoryCustomCode}
                              placeholder={t(
                                'adminDashboard.cfgItemCodePlaceholder',
                              )}
                              onChange={(e) =>
                                setInventoryCustomCode(e.target.value)
                              }
                            />

                            <input
                              className="admin-dashboard__search-input"
                              type="number"
                              min={1}
                              value={inventoryCount}
                              onChange={(e) =>
                                setInventoryCount(Number(e.target.value) || 1)
                              }
                            />

                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={inventoryWorn}
                                onChange={(e) =>
                                  setInventoryWorn(e.target.checked)
                                }
                              />
                              <span>{t('adminDashboard.cfgWorn')}</span>
                            </label>
                            <label className="admin-dashboard__cfg-check">
                              <input
                                type="checkbox"
                                checked={inventoryWornLeft}
                                onChange={(e) =>
                                  setInventoryWornLeft(e.target.checked)
                                }
                              />
                              <span>{t('adminDashboard.cfgWornLeft')}</span>
                            </label>

                            <button
                              type="button"
                              className="admin-dashboard__log-btn admin-dashboard__log-btn--primary admin-dashboard__log-btn--icon"
                              onClick={addStarterInventoryEntry}
                              aria-label={t('adminDashboard.add')}
                              title={t('adminDashboard.add')}
                            >
                              +
                            </button>
                          </div>

                          <div className="admin-dashboard__table-wrapper">
                            <table className="admin-dashboard__table">
                              <thead>
                                <tr>
                                  <th>{t('adminDashboard.cfgItem')}</th>
                                  <th>{t('adminDashboard.cfgCode')}</th>
                                  <th>{t('adminDashboard.cfgCount')}</th>
                                  <th>{t('adminDashboard.cfgFlags')}</th>
                                  <th>{t('adminDashboard.actions')}</th>
                                </tr>
                              </thead>
                              <tbody>
                                {cfgForm.starterInventory.map(
                                  (entry, index) => (
                                    <tr key={`${entry.baseId}-${index}`}>
                                      <td>
                                        {t(`items.${toCodeHex(entry.baseId)}`, {
                                          defaultValue:
                                            catalogNameByBaseId.get(
                                              entry.baseId,
                                            ) ||
                                            t('adminDashboard.cfgCustomItem'),
                                        })}
                                      </td>
                                      <td>{toCodeHex(entry.baseId)}</td>
                                      <td>{entry.count}</td>
                                      <td>
                                        {[
                                          entry.worn ? 'worn' : null,
                                          entry.wornLeft ? 'wornLeft' : null,
                                        ]
                                          .filter(Boolean)
                                          .join(', ') || '-'}
                                      </td>
                                      <td>
                                        <button
                                          type="button"
                                          className="admin-dashboard__kick-btn"
                                          onClick={() =>
                                            removeStarterInventoryEntry(index)
                                          }
                                        >
                                          {t('adminDashboard.remove')}
                                        </button>
                                      </td>
                                    </tr>
                                  ),
                                )}
                              </tbody>
                            </table>
                          </div>
                        </div>
                      </>
                    )}

                    {cfgEditorTab === 'json' && (
                      <textarea
                        className="admin-dashboard__cfg-editor"
                        value={cfgEditorText}
                        onChange={(e) => setCfgEditorText(e.target.value)}
                        spellCheck={false}
                        aria-label={t('adminDashboard.cfgEditorTitle')}
                      />
                    )}
                    <div className="admin-dashboard__cfg-status">
                      {cfgEditorStatus}
                    </div>
                  </div>
                )}

                {activeMenuSurface === 'sidebar' && activeTab === 'respawn' && (
                  <>
                    {!capabilities.canManageRespawn && (
                      <p className="admin-dashboard__no-players">
                        {t('adminDashboard.noPermission')}
                      </p>
                    )}

                    {capabilities.canManageRespawn && (
                      <RespawnPanel
                        downedPlayers={downedPlayers}
                        moderationReason={moderationReason}
                        onModerationReasonChange={setModerationReason}
                        onRevivePlayer={(userId) => void revivePlayer(userId)}
                        nowTs={nowTs}
                        statusMsg={statusMsg}
                        loading={loading}
                      />
                    )}
                  </>
                )}

                {activeMenuSurface === 'sidebar' && activeTab === 'events' && (
                  <>
                    {!capabilities.canViewLogs && (
                      <p className="admin-dashboard__no-players">
                        {t('adminDashboard.logsDisabled')}
                      </p>
                    )}

                    {capabilities.canViewLogs && (
                      <EventsPanel
                        revivalEvents={revivalEvents}
                        eventTypeFilter={eventTypeFilter}
                        onEventTypeFilterChange={setEventTypeFilter}
                        eventLimit={eventLimit}
                        onEventLimitChange={setEventLimit}
                        onRefresh={() => void fetchRevivalEvents()}
                        loading={loading}
                      />
                    )}
                  </>
                )}
              </>
            )}
          </main>

          <aside className="admin-dashboard__rightbar">
            <div className="admin-dashboard__player-summary-card">
              <div className="admin-dashboard__player-summary-label">
                {t('adminDashboard.players')}
              </div>
              <div className="admin-dashboard__player-summary-value">
                {status?.online ?? 0}
                <span className="admin-dashboard__player-summary-max">
                  /{status?.maxPlayers ?? '?'}
                </span>
              </div>
            </div>

            <div className="admin-dashboard__rightbar-card">
              <input
                className="admin-dashboard__player-search"
                type="text"
                placeholder={t('adminDashboard.searchPlaceholder')}
                aria-label={t('adminDashboard.searchPlaceholder')}
                value={playerSearch}
                onChange={(e) => setPlayerSearch(e.target.value)}
              />

              {visibleRailPlayers.length === 0 ? (
                <p className="admin-dashboard__rightbar-empty">
                  {t('adminDashboard.noPlayers')}
                </p>
              ) : (
                <div className="admin-dashboard__player-rail">
                  {visibleRailPlayers.map((player) => (
                    <button
                      key={player.userId}
                      type="button"
                      className="admin-dashboard__player-rail-item"
                      onClick={() => {
                        setActiveTab('players');
                        setActiveMenuSurface('sidebar');
                      }}
                    >
                      <span className="admin-dashboard__player-rail-name">
                        {player.actorName || `userId=${player.userId}`}
                      </span>
                      <span className="admin-dashboard__player-rail-meta">
                        ID {player.userId}
                      </span>
                    </button>
                  ))}
                </div>
              )}
            </div>
          </aside>
        </div>
      </div>
    </div>
  );
};

export default AdminDashboard;
