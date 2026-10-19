export interface AdminPlayerSearchEntry {
  userId: number;
  actorName?: string;
  ip?: string;
}

export interface AdminPosition {
  x: number;
  y: number;
  z: number;
}

export const formatAdminUptime = (seconds: number): string => {
  if (seconds < 60) return `${seconds}s`;
  if (seconds < 3600) return `${Math.floor(seconds / 60)}m ${seconds % 60}s`;
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  return `${h}h ${m}m`;
};

export const formatAdminPos = (
  pos: AdminPosition | number[] | undefined
): string => {
  if (!pos) return '-';
  if (Array.isArray(pos)) { return pos.map((v) => Math.round(Number(v))).join(', '); }
  return `${Math.round(pos.x)}, ${Math.round(pos.y)}, ${Math.round(pos.z)}`;
};

export const formatAdminTime = (ts: number): string =>
  new Date(ts).toLocaleTimeString();

export const filterAdminPlayers = <T extends AdminPlayerSearchEntry>(
  players: T[],
  search: string
): T[] => {
  const query = search.trim().toLowerCase();
  if (!query) return players;

  return players.filter((player) => {
    return (
      String(player.userId).includes(query) ||
      (player.actorName || '').toLowerCase().includes(query) ||
      (player.ip || '').includes(query)
    );
  });
};

export const isValidDiscordSnowflake = (value: string): boolean => {
  const trimmed = value.trim();
  return trimmed === '' || /^\d+$/.test(trimmed);
};

export const MAX_MODERATION_REASON_LENGTH = 200;

export const isValidModerationReason = (value: string): boolean =>
  value.trim().length <= MAX_MODERATION_REASON_LENGTH;

const SUPPORTED_WHITELIST_IDENTIFIER_TYPES = new Set([
  'discord',
  'steam',
  'license',
  'licenseea',
  'live',
  'xblive',
  'fal'
]);

export const isValidWhitelistIdentifier = (value: string): boolean => {
  const trimmed = value.trim();
  if (!trimmed) return false;

  const colonIndex = trimmed.indexOf(':');
  if (colonIndex < 1 || colonIndex >= trimmed.length - 1) return false;

  const type = trimmed.slice(0, colonIndex).trim().toLowerCase();
  const identifierValue = trimmed.slice(colonIndex + 1).trim();
  if (!identifierValue) return false;

  return SUPPORTED_WHITELIST_IDENTIFIER_TYPES.has(type);
};
