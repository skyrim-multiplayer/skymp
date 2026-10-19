const PING_GOOD = 80;
const PING_OK = 150;

export type SortKey = 'name' | 'players' | 'ping';

export interface ServerListViewEntry {
  id: string;
  name: string;
  ip: string;
  players: number;
  maxPlayers: number;
  ping: number | null;
  tags?: string[];
}

export const pingClass = (ping: number | null): string => {
  if (ping === null) return 'server-list__ping--unknown';
  if (ping <= PING_GOOD) return 'server-list__ping--good';
  if (ping <= PING_OK) return 'server-list__ping--ok';
  return 'server-list__ping--bad';
};

export const pingLabel = (ping: number | null): string => {
  if (ping === null) return '–';
  return `${ping}ms`;
};

export const isValidPort = (port: number): boolean => {
  return Number.isInteger(port) && port >= 1 && port <= 65535;
};

export const isValidHostOrIp = (value: string): boolean => {
  const input = value.trim();
  if (!input) return false;

  const ipv4 = /^(?:\d{1,3}\.){3}\d{1,3}$/;
  const hostname =
    /^(?=.{1,253}$)[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?(?:\.[a-zA-Z0-9](?:[a-zA-Z0-9-]{0,61}[a-zA-Z0-9])?)*$/;

  if (ipv4.test(input)) {
    return input
      .split('.')
      .every((octet) => Number(octet) >= 0 && Number(octet) <= 255);
  }

  return hostname.test(input);
};

export const collectServerTags = <T extends ServerListViewEntry>(
  servers: T[]
): string[] => {
  const tags = new Set<string>();
  servers.forEach((server) => {
    (server.tags || []).forEach((tag) => {
      const normalized = String(tag).trim();
      if (normalized) tags.add(normalized);
    });
  });
  return Array.from(tags).sort((a, b) => a.localeCompare(b));
};

export const getVisibleServers = <T extends ServerListViewEntry>(
  servers: T[],
  search: string,
  sortKey: SortKey,
  showFull: boolean,
  options?: {
    favoriteIds?: Set<string>;
    onlyFavorites?: boolean;
    requiredTag?: string;
  }
): T[] => {
  const query = search.trim().toLowerCase();
  const favoriteIds = options?.favoriteIds || new Set<string>();
  const onlyFavorites = Boolean(options?.onlyFavorites);
  const requiredTag = (options?.requiredTag || '').trim().toLowerCase();

  return servers
    .filter((server) => {
      if (!showFull && server.players >= server.maxPlayers) return false;
      if (onlyFavorites && !favoriteIds.has(server.id)) return false;
      if (
        requiredTag &&
        !(server.tags || []).some(
          (tag) => String(tag).toLowerCase() === requiredTag
        )
      ) { return false; }
      if (!query) return true;
      return (
        server.name.toLowerCase().includes(query) || server.ip.includes(query)
      );
    })
    .sort((a, b) => {
      if (sortKey === 'players') return b.players - a.players;
      if (sortKey === 'ping') {
        if (a.ping === null) return 1;
        if (b.ping === null) return -1;
        return a.ping - b.ping;
      }
      return a.name.localeCompare(b.name);
    });
};
