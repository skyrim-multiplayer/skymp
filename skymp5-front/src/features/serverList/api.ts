export interface ServerEntryDto {
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

export type ReleaseChannel = 'stable' | 'beta' | 'nightly';

export interface LatestUpdateDto {
  version: string;
  downloadUrl?: string;
  channel?: ReleaseChannel | string;
  notes?: string[] | string;
  changelog?: string[] | string;
  releaseNotesUrl?: string;
}

const removeTrailingSlash = (value: string): string =>
  value.replace(/\/+$/, '');

const toServersUrl = (apiEndpoint: string): string => {
  const trimmed = apiEndpoint.trim();
  if (!trimmed) return '/api/servers';
  if (trimmed.startsWith('/')) { return `${removeTrailingSlash(trimmed)}/api/servers`; }
  if (trimmed.startsWith('http://') || trimmed.startsWith('https://')) {
    return `${removeTrailingSlash(trimmed)}/api/servers`;
  }
  return `http://${removeTrailingSlash(trimmed)}/api/servers`;
};

const toUpdateUrl = (
  apiEndpoint: string,
  releaseChannel: ReleaseChannel,
  currentVersion?: string
): string => {
  const trimmed = apiEndpoint.trim();
  const query = new URLSearchParams({ channel: releaseChannel });
  if (currentVersion) {
    query.set('currentVersion', currentVersion);
  }

  if (!trimmed) return `/api/update/latest?${query.toString()}`;

  const updatePath = `/api/update/latest?${query.toString()}`;
  if (trimmed.startsWith('/')) { return `${removeTrailingSlash(trimmed)}${updatePath}`; }
  if (trimmed.startsWith('http://') || trimmed.startsWith('https://')) {
    return `${removeTrailingSlash(trimmed)}${updatePath}`;
  }
  return `http://${removeTrailingSlash(trimmed)}${updatePath}`;
};

export const fetchServerList = async (
  apiEndpoint: string
): Promise<ServerEntryDto[]> => {
  const url = toServersUrl(apiEndpoint);
  const response = await fetch(url, {
    method: 'GET',
    headers: {
      Accept: 'application/json'
    }
  });

  if (!response.ok) {
    throw new Error(`server-list-api:${response.status}`);
  }

  const data = await response.json();
  if (!Array.isArray(data)) {
    throw new Error('server-list-api:invalid-payload');
  }

  return data as ServerEntryDto[];
};

export const fetchLatestUpdate = async (
  apiEndpoint: string,
  releaseChannel: ReleaseChannel,
  currentVersion?: string
): Promise<LatestUpdateDto | null> => {
  const url = toUpdateUrl(apiEndpoint, releaseChannel, currentVersion);
  const response = await fetch(url, {
    method: 'GET',
    headers: {
      Accept: 'application/json'
    }
  });

  if (!response.ok) {
    throw new Error(`launcher-update-api:${response.status}`);
  }

  const data = (await response.json()) as LatestUpdateDto;
  if (!data || typeof data.version !== 'string' || !data.version.trim()) {
    throw new Error('launcher-update-api:invalid-payload');
  }

  return data;
};
