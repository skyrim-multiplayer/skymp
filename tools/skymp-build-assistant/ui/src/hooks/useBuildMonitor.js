import { useCallback, useEffect, useMemo, useRef, useState } from 'react';

import {
  cancelBuildJob,
  getBuildJobs,
  getBuildProfiles,
  runBuildJob,
  setBuildCmakeOption,
} from '../api.js';

const TERMINAL_BUILD_STATUSES = new Set(['succeeded', 'failed', 'cancelled']);

export function useBuildMonitor({ onSettled } = {}) {
  const [profiles, setProfiles] = useState([]);
  const [snapshot, setSnapshot] = useState({
    activeJobId: null,
    queuedJobIds: [],
    jobs: [],
    summary: {
      activeState: 'idle',
      activeJobId: null,
      queued: 0,
      trackedJobs: 0,
      lastFinishedStatus: null,
      lastFinishedJobId: null,
    },
  });
  const [loading, setLoading] = useState(true);
  const [refreshing, setRefreshing] = useState(false);
  const [error, setError] = useState('');
  const sawLiveJobRef = useRef(false);
  const lastSettledJobIdRef = useRef(null);

  const refreshProfiles = useCallback(async () => {
    const data = await getBuildProfiles();
    setProfiles(data.profiles ?? []);
    return data;
  }, []);

  const refreshJobs = useCallback(async () => {
    const data = await getBuildJobs();
    setSnapshot(data);
    return data;
  }, []);

  const refreshBuilds = useCallback(
    async ({ includeProfiles = true, silent = false } = {}) => {
      if (silent) {
        setRefreshing(true);
      } else {
        setLoading(true);
      }

      try {
        setError('');
        if (includeProfiles) {
          await Promise.all([refreshProfiles(), refreshJobs()]);
        } else {
          await refreshJobs();
        }
      } catch (nextError) {
        setError(nextError.message || 'Failed to load build data.');
      } finally {
        setLoading(false);
        setRefreshing(false);
      }
    },
    [refreshJobs, refreshProfiles],
  );

  useEffect(() => {
    void refreshBuilds();
  }, [refreshBuilds]);

  const activeJob = useMemo(() => {
    return snapshot.jobs.find((job) => job.id === snapshot.activeJobId) ?? null;
  }, [snapshot.activeJobId, snapshot.jobs]);

  const queuedJobs = useMemo(() => {
    const queuedIds = new Set(snapshot.queuedJobIds ?? []);
    return snapshot.jobs.filter((job) => queuedIds.has(job.id));
  }, [snapshot.jobs, snapshot.queuedJobIds]);

  const recentJobs = useMemo(() => {
    return snapshot.jobs.filter((job) => job.id !== snapshot.activeJobId).slice(0, 8);
  }, [snapshot.activeJobId, snapshot.jobs]);

  const hasLiveBuild = Boolean(activeJob) || (snapshot.queuedJobIds?.length ?? 0) > 0;

  useEffect(() => {
    if (!hasLiveBuild) {
      return undefined;
    }

    const timer = setInterval(() => {
      void refreshJobs();
    }, 2000);

    return () => clearInterval(timer);
  }, [hasLiveBuild, refreshJobs]);

  useEffect(() => {
    if (hasLiveBuild) {
      sawLiveJobRef.current = true;
      return;
    }

    if (!sawLiveJobRef.current) {
      return;
    }

    sawLiveJobRef.current = false;
    const latestTerminalJob = snapshot.jobs.find((job) => TERMINAL_BUILD_STATUSES.has(job.status));
    if (!latestTerminalJob || latestTerminalJob.id === lastSettledJobIdRef.current) {
      return;
    }

    lastSettledJobIdRef.current = latestTerminalJob.id;
    void refreshProfiles();
    onSettled?.(latestTerminalJob);
  }, [hasLiveBuild, onSettled, refreshProfiles, snapshot.jobs]);

  const runProfile = useCallback(async (profileId, options = {}) => {
    const response = await runBuildJob(profileId, options);
    setSnapshot(response.snapshot);
    return response;
  }, []);

  const cancelJob = useCallback(async (jobId = null) => {
    const response = await cancelBuildJob(jobId);
    setSnapshot(response.snapshot);
    return response;
  }, []);

  const updateCmakeOption = useCallback(async (optionKey, enabled) => {
    const response = await setBuildCmakeOption(optionKey, enabled);
    setProfiles(response.profiles ?? []);
    return response;
  }, []);

  return {
    profiles,
    snapshot,
    activeJob,
    queuedJobs,
    recentJobs,
    loading,
    refreshing,
    error,
    refreshBuilds,
    runProfile,
    cancelJob,
    updateCmakeOption,
  };
}
