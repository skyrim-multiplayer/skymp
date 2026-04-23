import { useCallback, useEffect, useState } from 'react';

import {
  getRuntimeStatus,
  launchRuntimeRequest,
  openRuntimeFolderRequest,
  stopRuntimeRequest,
} from '../api.js';

const DEFAULT_RUNTIME_SNAPSHOT = {
  platform: '',
  state: 'unknown',
  message: 'Runtime status has not been loaded yet.',
  checkedAt: '',
  running: false,
  canLaunch: false,
  canStop: false,
  processName: 'SkyrimSE.exe',
  processCount: 0,
  processIds: [],
  skyrimRoot: '',
  skyrimRootExists: false,
  skyrimExePath: '',
  skyrimExeExists: false,
  skseLoaderPath: '',
  skseLoaderExists: false,
  canOpenFolder: false,
};

export function useRuntimeMonitor() {
  const [runtime, setRuntime] = useState(DEFAULT_RUNTIME_SNAPSHOT);
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [error, setError] = useState('');

  const refreshRuntime = useCallback(async ({ silent = false } = {}) => {
    if (silent) {
      setRefreshing(true);
    } else {
      setLoading(true);
    }

    try {
      setError('');
      const data = await getRuntimeStatus();
      setRuntime(data ?? DEFAULT_RUNTIME_SNAPSHOT);
      return data;
    } catch (nextError) {
      setError(nextError.message || 'Failed to load runtime status.');
      return null;
    } finally {
      setLoading(false);
      setRefreshing(false);
    }
  }, []);

  useEffect(() => {
    void refreshRuntime();
  }, [refreshRuntime]);

  useEffect(() => {
    const intervalMs = runtime.running ? 2000 : 5000;
    const timer = setInterval(() => {
      void refreshRuntime({ silent: true });
    }, intervalMs);
    return () => clearInterval(timer);
  }, [refreshRuntime, runtime.running]);

  const requestLaunch = useCallback(async () => {
    setError('');
    const response = await launchRuntimeRequest();
    if (response?.status) {
      setRuntime(response.status);
    }
    return response;
  }, []);

  const requestStop = useCallback(async () => {
    setError('');
    const response = await stopRuntimeRequest();
    if (response?.status) {
      setRuntime(response.status);
    }
    return response;
  }, []);

  const requestOpenFolder = useCallback(async () => {
    setError('');
    const response = await openRuntimeFolderRequest();
    if (response?.status) {
      setRuntime(response.status);
    }
    return response;
  }, []);

  return {
    runtime,
    loading,
    refreshing,
    error,
    refreshRuntime,
    requestLaunch,
    requestOpenFolder,
    requestStop,
  };
}
