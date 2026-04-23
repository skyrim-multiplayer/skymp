import { useCallback, useEffect, useState } from 'react';

import { getModsSnapshot, reorderModsRequest, toggleModRequest } from '../api.js';

const DEFAULT_MODS_SNAPSHOT = {
  checkedAt: '',
  dataDir: {
    path: '',
    exists: false,
  },
  pluginsTxt: {
    path: '',
    exists: false,
    readable: false,
    readError: '',
    duplicatePluginNames: [],
    conflictingPluginNames: [],
    trackedPlugins: 0,
  },
  runtime: {
    blocked: false,
    reason: '',
  },
  mods: [],
  summary: {
    total: 0,
    enabled: 0,
    disabled: 0,
    inventoryOnly: 0,
    attention: 0,
    toggleable: 0,
    byKind: {},
  },
};

export function useModsMonitor() {
  const [snapshot, setSnapshot] = useState(DEFAULT_MODS_SNAPSHOT);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [reordering, setReordering] = useState(false);
  const [togglingModId, setTogglingModId] = useState('');
  const [error, setError] = useState('');

  const refreshMods = useCallback(async ({ silent = false } = {}) => {
    if (silent) {
      setRefreshing(true);
    } else {
      setLoading(true);
    }

    try {
      setError('');
      const data = await getModsSnapshot();
      setSnapshot(data ?? DEFAULT_MODS_SNAPSHOT);
      return data;
    } catch (nextError) {
      setError(nextError.message || 'Failed to load mods data.');
      return null;
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    void refreshMods();
  }, [refreshMods]);

  const toggleMod = useCallback(async (modId, enabled) => {
    setTogglingModId(modId);
    setError('');
    try {
      const response = await toggleModRequest(modId, enabled);
      if (response?.snapshot) {
        setSnapshot(response.snapshot);
      }
      return response;
    } finally {
      setTogglingModId('');
    }
  }, []);

  const reorderMods = useCallback(async (orderedModIds) => {
    setReordering(true);
    setError('');
    try {
      const response = await reorderModsRequest(orderedModIds);
      if (response?.snapshot) {
        setSnapshot(response.snapshot);
      }
      return response;
    } finally {
      setReordering(false);
    }
  }, []);

  return {
    snapshot,
    mods: snapshot.mods ?? [],
    summary: snapshot.summary ?? DEFAULT_MODS_SNAPSHOT.summary,
    loading,
    refreshing,
    reordering,
    togglingModId,
    error,
    refreshMods,
    reorderMods,
    toggleMod,
  };
}
