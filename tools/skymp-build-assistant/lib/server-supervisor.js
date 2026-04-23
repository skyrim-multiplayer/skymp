import { execFileSync, spawn } from 'child_process';
import fs from 'fs';
import path from 'path';

import { defaultServerLogPath, defaultServerStatePath } from './paths.js';

const PROCESS_POLL_INTERVAL_MS = 500;
const MAX_LOG_LINES = 800;
const MAX_LOG_BYTES = 256 * 1024;

function nowIso() {
  return new Date().toISOString();
}

function createServerError(message, code, status = null) {
  const error = new Error(message);
  error.code = code;
  error.status = status;
  return error;
}

function serverErrorCodeForState(state) {
  switch (state) {
    case 'unsupported':
      return 'SERVER_UNSUPPORTED';
    case 'build_dir_missing':
      return 'SERVER_BUILD_DIR_MISSING';
    case 'launch_script_missing':
      return 'SERVER_LAUNCH_SCRIPT_MISSING';
    default:
      return 'SERVER_NOT_READY';
  }
}

function isPidRunning(pid) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return false;
  }
  try {
    process.kill(pid, 0);
    return true;
  } catch {
    return false;
  }
}

function killProcessTree(pid) {
  if (!Number.isInteger(pid) || pid <= 0) {
    return;
  }

  try {
    if (process.platform === 'win32') {
      execFileSync('taskkill.exe', ['/PID', String(pid), '/T', '/F'], {
        stdio: ['ignore', 'ignore', 'ignore'],
      });
      return;
    }
    process.kill(pid, 'SIGTERM');
  } catch {
    // Best effort only.
  }
}

function readServerState(statePath) {
  if (!fs.existsSync(statePath)) {
    return null;
  }

  try {
    const raw = fs.readFileSync(statePath, 'utf8');
    return JSON.parse(raw);
  } catch {
    return null;
  }
}

function writeServerState(statePath, state) {
  fs.mkdirSync(path.dirname(statePath), { recursive: true });
  fs.writeFileSync(statePath, JSON.stringify(state, null, 2));
}

function getLaunchScriptPath(buildDir) {
  if (!buildDir) {
    return '';
  }
  return path.join(buildDir, 'launch_server.bat');
}

function appendSystemLog(logPath, message) {
  fs.mkdirSync(path.dirname(logPath), { recursive: true });
  fs.appendFileSync(logPath, `[skymp-dev ${nowIso()}] ${message}\n`);
}

function parseLogTail(logPath) {
  if (!logPath || !fs.existsSync(logPath)) {
    return {
      exists: false,
      lines: [],
      truncated: false,
      sizeBytes: 0,
    };
  }

  const sizeBytes = fs.statSync(logPath).size;
  const startByte = Math.max(0, sizeBytes - MAX_LOG_BYTES);
  const bytesToRead = sizeBytes - startByte;
  const fileDescriptor = fs.openSync(logPath, 'r');

  try {
    const buffer = Buffer.alloc(bytesToRead);
    fs.readSync(fileDescriptor, buffer, 0, bytesToRead, startByte);
    let text = buffer.toString('utf8');

    if (startByte > 0) {
      const firstNewline = text.indexOf('\n');
      text = firstNewline === -1 ? '' : text.slice(firstNewline + 1);
    }

    const lines = text
      .replace(/\r/g, '')
      .split('\n')
      .map((line) => line.trimEnd())
      .filter((line) => line.length > 0);

    return {
      exists: true,
      lines: lines.slice(-MAX_LOG_LINES),
      truncated: startByte > 0 || lines.length > MAX_LOG_LINES,
      sizeBytes,
    };
  } finally {
    fs.closeSync(fileDescriptor);
  }
}

function getServerStateMessage({
  state,
  buildDir,
  launchScriptPath,
  processId,
  startedAt,
  logExists,
}) {
  switch (state) {
    case 'unsupported':
      return 'Server control is currently supported on Windows only.';
    case 'running':
      return processId
        ? `Server is running from ${launchScriptPath} (pid ${processId}).`
        : 'Server is currently running.';
    case 'build_dir_missing':
      return buildDir
        ? `Configured build directory does not exist: ${buildDir}`
        : 'Build directory is not configured.';
    case 'launch_script_missing':
      return `Server launch script was not found at ${launchScriptPath}. Build the server first.`;
    case 'ready_to_start':
    default:
      if (startedAt && logExists) {
        return 'Server is not running right now. Showing the latest captured log tail from the previous session.';
      }
      return 'Server is ready to start through build/launch_server.bat.';
  }
}

function normalizeServerState(statePath, state) {
  if (!state || !Number.isInteger(state.pid) || state.pid <= 0) {
    return state;
  }

  if (isPidRunning(state.pid)) {
    return state;
  }

  const nextState = {
    ...state,
    lastPid: state.pid,
    pid: null,
    stoppedAt: state.stoppedAt || nowIso(),
  };
  writeServerState(statePath, nextState);
  return nextState;
}

function buildServerSnapshot(runtime, state, logTail) {
  const repoRoot = runtime?.repoRoot || runtime?.ctx?.repoRoot || process.cwd();
  const buildDir = runtime?.ctx?.buildDir ? String(runtime.ctx.buildDir) : '';
  const launchScriptPath = state?.launchScriptPath || getLaunchScriptPath(buildDir);
  const buildDirExists = Boolean(buildDir) && fs.existsSync(buildDir);
  const launchScriptExists = Boolean(launchScriptPath) && fs.existsSync(launchScriptPath);
  const logPath = state?.logPath || defaultServerLogPath(repoRoot);
  const processId = Number.isInteger(state?.pid) && state.pid > 0 ? state.pid : null;
  const running = processId != null && isPidRunning(processId);

  let viewState = 'ready_to_start';
  if (process.platform !== 'win32') {
    viewState = 'unsupported';
  } else if (!buildDirExists) {
    viewState = 'build_dir_missing';
  } else if (running) {
    viewState = 'running';
  } else if (!launchScriptExists) {
    viewState = 'launch_script_missing';
  }

  return {
    platform: process.platform,
    state: viewState,
    message: getServerStateMessage({
      state: viewState,
      buildDir,
      launchScriptPath,
      processId,
      startedAt: state?.startedAt,
      logExists: logTail.exists,
    }),
    checkedAt: nowIso(),
    running,
    canStart: viewState === 'ready_to_start',
    canStop: running,
    buildDir,
    buildDirExists,
    launchScriptPath,
    launchScriptExists,
    processId,
    lastPid:
      !running && Number.isInteger(state?.lastPid) && state.lastPid > 0
        ? state.lastPid
        : !running && processId
          ? processId
          : null,
    startedAt: state?.startedAt || '',
    stoppedAt: running ? '' : state?.stoppedAt || '',
    logPath,
    logExists: logTail.exists,
    logLines: logTail.lines,
    logLineCount: logTail.lines.length,
    logTruncated: logTail.truncated,
    logSizeBytes: logTail.sizeBytes,
  };
}

async function sleep(ms) {
  await new Promise((resolve) => setTimeout(resolve, ms));
}

async function waitForServerSnapshot(runtime, predicate, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  let snapshot = getServerStatus(runtime);

  while (Date.now() < deadline) {
    if (predicate(snapshot)) {
      return snapshot;
    }
    await sleep(PROCESS_POLL_INTERVAL_MS);
    snapshot = getServerStatus(runtime);
  }

  return snapshot;
}

async function spawnDetachedServer(launchScriptPath, buildDir, logPath) {
  return new Promise((resolve, reject) => {
    let logFd = null;
    try {
      logFd = fs.openSync(logPath, 'a');
    } catch (error) {
      reject(error);
      return;
    }

    const child = spawn('cmd.exe', ['/d', '/s', '/c', path.basename(launchScriptPath)], {
      cwd: buildDir,
      detached: true,
      stdio: ['ignore', logFd, logFd],
      windowsHide: true,
    });

    function cleanupLogFd() {
      if (logFd == null) {
        return;
      }
      try {
        fs.closeSync(logFd);
      } catch {
        // Best effort only.
      }
      logFd = null;
    }

    child.once('error', (error) => {
      cleanupLogFd();
      reject(error);
    });

    child.once('spawn', () => {
      cleanupLogFd();
      child.unref();
      resolve(child);
    });
  });
}

export function getServerStatus(runtime) {
  const repoRoot = runtime?.repoRoot || runtime?.ctx?.repoRoot || process.cwd();
  const statePath = defaultServerStatePath(repoRoot);
  const state = normalizeServerState(statePath, readServerState(statePath));
  const logPath = state?.logPath || defaultServerLogPath(repoRoot);
  const logTail = parseLogTail(logPath);
  return buildServerSnapshot(runtime, state, logTail);
}

export async function startServer(runtime) {
  const repoRoot = runtime?.repoRoot || runtime?.ctx?.repoRoot || process.cwd();
  const statePath = defaultServerStatePath(repoRoot);
  const status = getServerStatus(runtime);

  if (status.state === 'running') {
    return {
      action: 'already_running',
      status,
    };
  }

  if (status.state !== 'ready_to_start') {
    throw createServerError(status.message, serverErrorCodeForState(status.state), status);
  }

  fs.mkdirSync(path.dirname(status.logPath), { recursive: true });
  fs.writeFileSync(status.logPath, '');
  appendSystemLog(status.logPath, `Starting server via ${status.launchScriptPath}`);

  let child = null;
  try {
    child = await spawnDetachedServer(status.launchScriptPath, status.buildDir, status.logPath);
  } catch (error) {
    appendSystemLog(status.logPath, `Failed to launch server: ${error.message || String(error)}`);
    const failedStatus = getServerStatus(runtime);
    throw createServerError(
      error.message || 'Could not launch build/launch_server.bat.',
      'SERVER_START_FAILED',
      failedStatus,
    );
  }

  writeServerState(statePath, {
    pid: child?.pid ?? null,
    lastPid: null,
    startedAt: nowIso(),
    stoppedAt: '',
    logPath: status.logPath,
    buildDir: status.buildDir,
    launchScriptPath: status.launchScriptPath,
  });

  const nextStatus = await waitForServerSnapshot(runtime, (snapshot) => snapshot.running, 1500);
  if (!nextStatus.running || !nextStatus.processId) {
    const failedStatus = getServerStatus(runtime);
    const detail = failedStatus.logLines.at(-1) ? ` ${failedStatus.logLines.at(-1)}` : '';
    throw createServerError(
      `Server process exited immediately after launch.${detail}`,
      'SERVER_START_FAILED',
      failedStatus,
    );
  }

  return {
    action: 'start_requested',
    status: nextStatus,
  };
}

export async function stopServer(runtime) {
  const repoRoot = runtime?.repoRoot || runtime?.ctx?.repoRoot || process.cwd();
  const statePath = defaultServerStatePath(repoRoot);
  const status = getServerStatus(runtime);

  if (status.state === 'unsupported') {
    throw createServerError(status.message, serverErrorCodeForState(status.state), status);
  }

  if (!status.processId) {
    return {
      action: 'not_running',
      stoppedProcessCount: 0,
      status,
    };
  }

  appendSystemLog(status.logPath, 'Stop requested from dashboard.');
  killProcessTree(status.processId);

  const nextStatus = await waitForServerSnapshot(runtime, (snapshot) => !snapshot.running, 5000);
  const nextState = readServerState(statePath);
  writeServerState(statePath, {
    ...(nextState ?? {}),
    pid: null,
    lastPid: status.processId,
    startedAt: status.startedAt || nextState?.startedAt || '',
    stoppedAt: nowIso(),
    logPath: status.logPath,
    buildDir: status.buildDir,
    launchScriptPath: status.launchScriptPath,
  });

  return {
    action: 'stop_requested',
    stoppedProcessCount: 1,
    status: getServerStatus(runtime),
  };
}
