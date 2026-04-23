export async function fetchJson(url, options = {}) {
  const response = await fetch(url, options);
  const text = await response.text();
  const data = text ? JSON.parse(text) : null;

  if (!response.ok) {
    const error = new Error(data?.error || `Request failed (${response.status})`);
    error.details = data?.details ?? [];
    error.code = data?.code ?? null;
    error.profile = data?.profile ?? null;
    error.status = data?.status ?? null;
    throw error;
  }

  return data;
}

export function getMeta() {
  return fetchJson('/api/meta');
}

export function getStatus() {
  return fetchJson('/api/status');
}

export function getDoctor() {
  return fetchJson('/api/doctor');
}

export function getConfig() {
  return fetchJson('/api/config');
}

export function saveConfig(values) {
  return fetchJson('/api/config', {
    method: 'PUT',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ values }),
  });
}

export function runSyncRequest({ dryRun = false, onlyId = null } = {}) {
  return fetchJson('/api/sync', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ dryRun, onlyId }),
  });
}

export function getBuildProfiles() {
  return fetchJson('/api/build/profiles');
}

export function getBuildJobs() {
  return fetchJson('/api/build/jobs');
}

export function setBuildCmakeOption(optionKey, enabled) {
  return fetchJson('/api/build/cmake-options', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ optionKey, enabled }),
  });
}

export function runBuildJob(profileId, { confirmDestructive = false } = {}) {
  return fetchJson('/api/build/run', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ profileId, confirmDestructive }),
  });
}

export function cancelBuildJob(jobId = null) {
  return fetchJson('/api/build/cancel', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ jobId }),
  });
}

export function getRuntimeStatus() {
  return fetchJson('/api/runtime');
}

export function launchRuntimeRequest() {
  return fetchJson('/api/runtime/launch', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
  });
}

export function stopRuntimeRequest() {
  return fetchJson('/api/runtime/stop', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
  });
}

export function openRuntimeFolderRequest() {
  return fetchJson('/api/runtime/open-folder', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
  });
}

export function getModsSnapshot() {
  return fetchJson('/api/mods');
}

export function toggleModRequest(modId, enabled) {
  return fetchJson('/api/mods/toggle', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ modId, enabled }),
  });
}

export function reorderModsRequest(orderedModIds) {
  return fetchJson('/api/mods/reorder', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify({ orderedModIds }),
  });
}
