import { execFileSync, spawn } from 'child_process';
import { EventEmitter } from 'events';
import fs from 'fs';

import { getBuildProfile, listBuildProfiles } from './build-profiles.js';

const TERMINAL_STATUSES = new Set(['succeeded', 'failed', 'cancelled']);
const MAX_LOG_ENTRIES = 800;
const MAX_JOBS = 25;

function nowIso() {
  return new Date().toISOString();
}

function normalizeLogLines(chunk) {
  return String(chunk)
    .replace(/\r/g, '')
    .split('\n')
    .map((line) => line.trimEnd())
    .filter((line) => line.length > 0);
}

function cloneValue(value) {
  return JSON.parse(JSON.stringify(value));
}

function createJobId(counter) {
  return `job-${Date.now()}-${counter}`;
}

function isTerminalStatus(status) {
  return TERMINAL_STATUSES.has(status);
}

function killProcessTree(pid) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return;
  }

  try {
    if (process.platform === 'win32') {
      execFileSync('taskkill', ['/PID', String(pid), '/T', '/F'], {
        stdio: 'ignore',
      });
      return;
    }
    process.kill(pid, 'SIGTERM');
  } catch {
    // Best effort only.
  }
}

function createJobSnapshot(job) {
  return cloneValue(job);
}

function createQueuedStep(step, index) {
  return {
    id: step.id ?? `step-${index + 1}`,
    label: step.label,
    cwd: step.cwd,
    command: step.command,
    status: 'pending',
    startedAt: null,
    endedAt: null,
    exitCode: null,
  };
}

function createJob(profile, requestedBy, counter) {
  return {
    id: createJobId(counter),
    profileId: profile.id,
    profileLabel: profile.label,
    description: profile.description,
    category: profile.category,
    executionMode: profile.executionMode,
    longRunningCapable: profile.longRunningCapable,
    dependencies: profile.dependencies,
    status: 'queued',
    requestedBy,
    requestedAt: nowIso(),
    startedAt: null,
    endedAt: null,
    exitCode: null,
    disabledReason: profile.disabledReason,
    requirements: profile.requirements,
    expectedOutputs: profile.expectedOutputs,
    missingOutputs: [],
    failureReason: null,
    cancelRequested: false,
    queuePosition: 0,
    steps: profile.steps.map(createQueuedStep),
    logs: [],
  };
}

function createShellCommand(step) {
  if (process.platform === 'win32') {
    return spawn('powershell.exe', ['-NoProfile', '-Command', step.command], {
      cwd: step.cwd,
      env: { ...process.env, ...(step.env ?? {}) },
      windowsHide: true,
    });
  }

  return spawn('/bin/bash', ['-lc', step.command], {
    cwd: step.cwd,
    env: { ...process.env, ...(step.env ?? {}) },
  });
}

class BuildOrchestrator extends EventEmitter {
  constructor() {
    super();
    this.jobs = [];
    this.activeJobId = null;
    this.queue = [];
    this.currentChild = null;
    this.jobCounter = 0;
  }

  listProfiles(runtime) {
    return listBuildProfiles(runtime);
  }

  getProfile(profileId, runtime) {
    return getBuildProfile(profileId, runtime);
  }

  getSnapshot() {
    const jobs = [...this.jobs].sort((left, right) => {
      return new Date(right.requestedAt).getTime() - new Date(left.requestedAt).getTime();
    });

    return {
      activeJobId: this.activeJobId,
      queuedJobIds: [...this.queue],
      jobs: jobs.map(createJobSnapshot),
    };
  }

  getJob(jobId) {
    const job = this.jobs.find((candidate) => candidate.id === jobId);
    return job ? createJobSnapshot(job) : null;
  }

  async runProfile(
    profileId,
    runtime,
    { requestedBy = 'dashboard', confirmDestructive = false } = {},
  ) {
    const profile = this.getProfile(profileId, runtime);
    if (!profile) {
      const error = new Error(`Unknown build profile: ${profileId}`);
      error.code = 'UNKNOWN_BUILD_PROFILE';
      throw error;
    }

    if (!profile.enabled) {
      const error = new Error(profile.disabledReason || `Profile ${profileId} is not currently runnable.`);
      error.code = 'BUILD_PROFILE_DISABLED';
      error.profile = profile;
      throw error;
    }

    if (profile.destructive && !confirmDestructive) {
      const error = new Error(
        profile.confirmMessage ||
          `Profile ${profileId} is destructive. Re-run with explicit confirmation.`,
      );
      error.code = 'BUILD_PROFILE_CONFIRMATION_REQUIRED';
      error.profile = profile;
      throw error;
    }

    this.jobCounter += 1;
    const job = createJob(profile, requestedBy, this.jobCounter);
    this.jobs.push(job);
    this.trimHistory();

    if (this.activeJobId) {
      this.queue.push(job.id);
      this.updateQueuePositions();
      this.emitUpdate();
      return this.getJob(job.id);
    }

    this.updateQueuePositions();
    this.emitUpdate();
    void this.startJob(job, profile);
    return this.getJob(job.id);
  }

  async waitForJob(jobId) {
    const job = this.jobs.find((candidate) => candidate.id === jobId);
    if (!job) {
      throw new Error(`Unknown build job: ${jobId}`);
    }

    if (isTerminalStatus(job.status)) {
      return createJobSnapshot(job);
    }

    return new Promise((resolve) => {
      const handler = (snapshot) => {
        const nextJob = snapshot.jobs.find((candidate) => candidate.id === jobId);
        if (!nextJob || !isTerminalStatus(nextJob.status)) {
          return;
        }
        this.off('update', handler);
        resolve(nextJob);
      };
      this.on('update', handler);
    });
  }

  cancelJob(jobId = null) {
    const effectiveJobId = jobId ?? this.activeJobId ?? this.queue[0] ?? null;
    if (!effectiveJobId) {
      return {
        cancelled: false,
        reason: 'no_job',
      };
    }

    const queuedIndex = this.queue.indexOf(effectiveJobId);
    if (queuedIndex !== -1) {
      this.queue.splice(queuedIndex, 1);
      const queuedJob = this.jobs.find((candidate) => candidate.id === effectiveJobId);
      if (queuedJob) {
        queuedJob.status = 'cancelled';
        queuedJob.failureReason = 'cancelled_before_start';
        queuedJob.endedAt = nowIso();
        this.appendLog(queuedJob, 'system', 'Cancelled before the build started.');
      }
      this.updateQueuePositions();
      this.emitUpdate();
      return {
        cancelled: true,
        source: 'queue',
        job: queuedJob ? createJobSnapshot(queuedJob) : null,
      };
    }

    if (this.activeJobId === effectiveJobId) {
      const activeJob = this.jobs.find((candidate) => candidate.id === effectiveJobId);
      if (!activeJob) {
        return { cancelled: false, reason: 'job_not_found' };
      }

      activeJob.cancelRequested = true;
      activeJob.failureReason = 'cancel_requested';
      this.appendLog(activeJob, 'system', 'Cancellation requested. Stopping active build...');

      if (this.currentChild?.pid) {
        killProcessTree(this.currentChild.pid);
      }

      this.emitUpdate();
      return {
        cancelled: true,
        source: 'active',
        job: createJobSnapshot(activeJob),
      };
    }

    return {
      cancelled: false,
      reason: 'job_not_found',
    };
  }

  trimHistory() {
    const terminalJobs = this.jobs.filter((job) => isTerminalStatus(job.status));
    if (terminalJobs.length <= MAX_JOBS) {
      return;
    }

    const removableCount = terminalJobs.length - MAX_JOBS;
    const terminalIdsToRemove = terminalJobs
      .sort((left, right) => new Date(left.requestedAt).getTime() - new Date(right.requestedAt).getTime())
      .slice(0, removableCount)
      .map((job) => job.id);

    this.jobs = this.jobs.filter((job) => !terminalIdsToRemove.includes(job.id));
  }

  updateQueuePositions() {
    for (const job of this.jobs) {
      job.queuePosition = 0;
    }

    this.queue.forEach((jobId, index) => {
      const job = this.jobs.find((candidate) => candidate.id === jobId);
      if (job) {
        job.queuePosition = index + 1;
      }
    });
  }

  emitUpdate() {
    this.emit('update', this.getSnapshot());
  }

  appendLog(job, stream, message) {
    const entries = normalizeLogLines(message);
    if (entries.length === 0) {
      return;
    }

    for (const entry of entries) {
      job.logs.push({
        timestamp: nowIso(),
        stream,
        message: entry,
      });
    }

    if (job.logs.length > MAX_LOG_ENTRIES) {
      job.logs.splice(0, job.logs.length - MAX_LOG_ENTRIES);
    }

    this.emit('log', {
      jobId: job.id,
      entries: entries.map((entry) => ({
        timestamp: nowIso(),
        stream,
        message: entry,
      })),
    });
  }

  async startNextQueuedJob() {
    if (this.activeJobId || this.queue.length === 0) {
      return;
    }

    const nextJobId = this.queue.shift();
    this.updateQueuePositions();
    if (!nextJobId) {
      this.emitUpdate();
      return;
    }

    const nextJob = this.jobs.find((candidate) => candidate.id === nextJobId);
    if (!nextJob || isTerminalStatus(nextJob.status)) {
      this.emitUpdate();
      return this.startNextQueuedJob();
    }

    this.emitUpdate();
    const runtimeProfile = {
      ...nextJob,
      steps: nextJob.steps.map((step) => ({
        ...step,
        cwd: step.cwd,
        command: step.command,
      })),
    };
    await this.startJob(nextJob, runtimeProfile);
  }

  async startJob(job, profile) {
    job.status = 'running';
    job.startedAt = nowIso();
    job.failureReason = null;
    this.activeJobId = job.id;
    this.emitUpdate();

    try {
      for (let index = 0; index < job.steps.length; index += 1) {
        const jobStep = job.steps[index];
        const profileStep = profile.steps[index];
        if (!profileStep) {
          continue;
        }

        jobStep.status = 'running';
        jobStep.startedAt = nowIso();
        this.appendLog(job, 'system', `Starting step: ${jobStep.label}`);
        this.emitUpdate();

        const exitResult = await this.runStep(job, profileStep);
        jobStep.endedAt = nowIso();
        jobStep.exitCode = exitResult.exitCode;

        if (job.cancelRequested) {
          jobStep.status = 'cancelled';
          job.status = 'cancelled';
          job.failureReason = 'cancelled';
          job.endedAt = nowIso();
          job.exitCode = null;
          this.appendLog(job, 'system', 'Build was cancelled.');
          this.emitUpdate();
          return;
        }

        if (exitResult.exitCode !== 0) {
          jobStep.status = 'failed';
          job.status = 'failed';
          job.failureReason = 'step_failed';
          job.exitCode = exitResult.exitCode;
          job.endedAt = nowIso();
          this.appendLog(
            job,
            'system',
            `Step failed: ${jobStep.label} (exit code ${exitResult.exitCode}).`,
          );
          this.emitUpdate();
          return;
        }

        jobStep.status = 'succeeded';
        this.appendLog(job, 'system', `Completed step: ${jobStep.label}`);
        this.emitUpdate();
      }

      const missingOutputs = job.expectedOutputs.filter((output) => !fs.existsSync(output.path));
      if (missingOutputs.length > 0) {
        job.status = 'failed';
        job.failureReason = 'missing_outputs';
        job.endedAt = nowIso();
        job.exitCode = 0;
        job.missingOutputs = missingOutputs;
        for (const output of missingOutputs) {
          this.appendLog(
            job,
            'system',
            `Expected output missing after build: ${output.label} (${output.path})`,
          );
        }
        this.emitUpdate();
        return;
      }

      job.status = 'succeeded';
      job.endedAt = nowIso();
      job.exitCode = 0;
      this.appendLog(job, 'system', 'Build completed successfully.');
      this.emitUpdate();
    } catch (error) {
      job.status = job.cancelRequested ? 'cancelled' : 'failed';
      job.failureReason = job.cancelRequested ? 'cancelled' : 'internal_error';
      job.endedAt = nowIso();
      job.exitCode = 1;
      this.appendLog(job, 'system', error?.message || String(error));
      this.emitUpdate();
    } finally {
      this.currentChild = null;
      this.activeJobId = null;
      this.emitUpdate();
      await this.startNextQueuedJob();
    }
  }

  async runStep(job, step) {
    const child = createShellCommand(step);
    this.currentChild = child;

    child.stdout?.on('data', (chunk) => {
      this.appendLog(job, 'stdout', chunk);
      this.emitUpdate();
    });

    child.stderr?.on('data', (chunk) => {
      this.appendLog(job, 'stderr', chunk);
      this.emitUpdate();
    });

    return new Promise((resolve, reject) => {
      child.on('error', (error) => {
        reject(error);
      });

      child.on('exit', async (code, signal) => {
        this.currentChild = null;
        if (signal && !job.cancelRequested) {
          this.appendLog(job, 'system', `Process exited with signal ${signal}.`);
        }

        if (job.cancelRequested) {
          resolve({ exitCode: 1, signal });
          return;
        }

        resolve({
          exitCode: code ?? 1,
          signal,
        });
      });
    });
  }
}

export const buildOrchestrator = new BuildOrchestrator();
