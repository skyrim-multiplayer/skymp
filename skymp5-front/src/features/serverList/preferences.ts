export interface LauncherServerRef {
  id?: string;
  ip: string;
  port: number;
}

export interface CachedServerEntry {
  id: string;
  name: string;
  ip: string;
  port: number;
  players: number;
  maxPlayers: number;
  ping: number | null;
  version: string;
  description?: string;
  tags?: string[];
  online: boolean;
  passwordProtected?: boolean;
}

const KEYS = {
  favorites: 'skymp.launcher.favorites',
  autoConnect: 'skymp.launcher.autoconnect',
  lastServer: 'skymp.launcher.lastServer',
  apiEndpoint: 'skymp.launcher.apiEndpoint',
  cachedServers: 'skymp.launcher.cachedServers',
  darkMode: 'skymp.launcher.darkMode',
  releaseChannel: 'skymp.launcher.releaseChannel',
  ignoredUpdates: 'skymp.launcher.ignoredUpdates'
};

type IgnoredUpdatesMap = Partial<Record<'stable' | 'beta' | 'nightly', string>>;

const canUseStorage = (): boolean => {
  try {
    return typeof window !== 'undefined' && !!window.localStorage;
  } catch {
    return false;
  }
};

const readJson = <T>(key: string, fallback: T): T => {
  if (!canUseStorage()) return fallback;

  try {
    const raw = window.localStorage.getItem(key);
    if (!raw) return fallback;
    return JSON.parse(raw) as T;
  } catch {
    return fallback;
  }
};

const writeJson = <T>(key: string, value: T): void => {
  if (!canUseStorage()) return;

  try {
    window.localStorage.setItem(key, JSON.stringify(value));
  } catch {
    // ignore quota/storage failures
  }
};

export const getFavoriteServerIds = (): string[] =>
  readJson<string[]>(KEYS.favorites, []);

export const setFavoriteServerIds = (ids: string[]): void => {
  writeJson(KEYS.favorites, ids);
};

export const getAutoConnectLast = (): boolean =>
  readJson<boolean>(KEYS.autoConnect, false);

export const setAutoConnectLast = (value: boolean): void => {
  writeJson(KEYS.autoConnect, value);
};

export const getLastServerRef = (): LauncherServerRef | null =>
  readJson<LauncherServerRef | null>(KEYS.lastServer, null);

export const setLastServerRef = (server: LauncherServerRef): void => {
  writeJson(KEYS.lastServer, server);
};

export const getLauncherApiEndpoint = (): string => {
  if (typeof window === 'undefined') return '';

  const fromStorage = readJson<string>(KEYS.apiEndpoint, '').trim();
  if (fromStorage) return fromStorage;

  const defaultByLocation = `${window.location.protocol}//${window.location.hostname}:7777`;
  return defaultByLocation;
};

export const setLauncherApiEndpoint = (value: string): void => {
  writeJson(KEYS.apiEndpoint, value.trim());
};

export const getCachedServers = (): CachedServerEntry[] =>
  readJson<CachedServerEntry[]>(KEYS.cachedServers, []);

export const setCachedServers = (servers: CachedServerEntry[]): void => {
  writeJson(KEYS.cachedServers, servers);
};

export const getLauncherDarkMode = (): boolean | null =>
  readJson<boolean | null>(KEYS.darkMode, null);

export const setLauncherDarkMode = (enabled: boolean): void => {
  writeJson(KEYS.darkMode, enabled);
};

export const getLauncherReleaseChannel = (): 'stable' | 'beta' | 'nightly' => {
  const value = readJson<string>(KEYS.releaseChannel, 'stable');
  if (value === 'beta' || value === 'nightly') return value;
  return 'stable';
};

export const setLauncherReleaseChannel = (
  channel: 'stable' | 'beta' | 'nightly'
): void => {
  writeJson(KEYS.releaseChannel, channel);
};

export const getLauncherIgnoredUpdateVersion = (
  channel: 'stable' | 'beta' | 'nightly'
): string | null => {
  const map = readJson<IgnoredUpdatesMap>(KEYS.ignoredUpdates, {});
  const value = map[channel];
  return typeof value === 'string' && value.trim() ? value.trim() : null;
};

export const setLauncherIgnoredUpdateVersion = (
  channel: 'stable' | 'beta' | 'nightly',
  version: string
): void => {
  const map = readJson<IgnoredUpdatesMap>(KEYS.ignoredUpdates, {});
  const next: IgnoredUpdatesMap = { ...map, [channel]: version.trim() };
  writeJson(KEYS.ignoredUpdates, next);
};

export const clearLauncherIgnoredUpdateVersion = (
  channel: 'stable' | 'beta' | 'nightly'
): void => {
  const map = readJson<IgnoredUpdatesMap>(KEYS.ignoredUpdates, {});
  if (!(channel in map)) return;

  const next: IgnoredUpdatesMap = { ...map };
  delete next[channel];
  writeJson(KEYS.ignoredUpdates, next);
};
