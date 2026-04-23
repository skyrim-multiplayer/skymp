import { useCallback, useEffect, useState } from 'react';

import { getServerStatus, startServerRequest, stopServerRequest } from '../api.js';

const DEFAULT_SERVER_SNAPSHOT = {
  platform: '',
  state: 'unknown',
  message: 'Server status has not been loaded yet.',
  checkedAt: '',
  running: false,
  canStart: false,
  canStop: false,
  buildDir: '',
  buildDirExists: false,
  launchScriptPath: '',
  launchScriptExists: false,
  processId: null,
  lastPid: null,
  startedAt: '',
  stoppedAt: '',
  logPath: '',
  logExists: false,
  logLines: [],
  logLineCount: 0,
  logTruncated: false,
  logSizeBytes: 0,
};

export function useServerMonitor() {
  const [server, setServer] = useState(DEFAULT_SERVER_SNAPSHOT);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [error, setError] = useState('');

  const refreshServer = useCallback(async ({ silent = false } = {}) => {
    if (silent) {
      setRefreshing(true);
    } else {
      setLoading(true);
    }

    try {
      setError('');
      const data = await getServerStatus();
      setServer(data ?? DEFAULT_SERVER_SNAPSHOT);
      return data;
    } catch (nextError) {
      setError(nextError.message || 'Failed to load server status.');
      return null;
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    void refreshServer();
  }, [refreshServer]);

  useEffect(() => {
    const intervalMs = server.running ? 2000 : 5000;
    const timer = setInterval(() => {
      void refreshServer({ silent: true });
    }, intervalMs);
    return () => clearInterval(timer);
  }, [refreshServer, server.running]);

  const requestStart = useCallback(async () => {
    setError('');
    const response = await startServerRequest();
    if (response?.status) {
      setServer(response.status);
    }
    return response;
  }, []);

  const requestStop = useCallback(async () => {
    setError('');
    const response = await stopServerRequest();
    if (response?.status) {
      setServer(response.status);
    }
    return response;
  }, []);

  return {
    server,
    loading,
    refreshing,
    error,
    refreshServer,
    requestStart,
    requestStop,
  };
}
