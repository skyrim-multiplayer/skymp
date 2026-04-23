import { execFileSync, spawn } from 'child_process';
import fs from 'fs';

const SKYRIM_PROCESS_NAME = 'SkyrimSE.exe';
const PROCESS_POLL_INTERVAL_MS = 500;

function createRuntimeError(message, code, status = null) {
  const error = new Error(message);
  error.code = code;
  error.status = status;
  return error;
}

function runtimeErrorCodeForState(state) {
  switch (state) {
    case 'unsupported':
      return 'RUNTIME_UNSUPPORTED';
    case 'skyrim_root_missing':
      return 'RUNTIME_SKYRIM_ROOT_MISSING';
    case 'bad_skyrim_root':
      return 'RUNTIME_BAD_SKYRIM_ROOT';
    case 'skse_missing':
      return 'RUNTIME_SKSE_MISSING';
    default:
      return 'RUNTIME_NOT_READY';
  }
}

function getRuntimeStateMessage({ state, processCount, skyrimRoot, skseLoaderPath }) {
  switch (state) {
    case 'unsupported':
      return 'Runtime supervision is currently supported on Windows only.';
    case 'running':
      return processCount === 1
        ? 'Skyrim is currently running.'
        : `Skyrim is currently running (${processCount} processes detected).`;
    case 'skyrim_root_missing':
      return 'Set Skyrim Root in Settings before launching Skyrim from the assistant.';
    case 'bad_skyrim_root':
      return `SkyrimSE.exe was not found under the configured Skyrim root: ${skyrimRoot}`;
    case 'skse_missing':
      return `SKSE loader was not found at ${skseLoaderPath}.`;
    case 'ready_to_launch':
      return 'Skyrim is ready to launch through SKSE.';
    default:
      return 'Runtime state is unknown.';
  }
}

function readWindowsGameProcesses() {
  if (process.platform !== 'win32') {
    return [];
  }

  try {
    const script = `
$procs = Get-CimInstance Win32_Process -Filter "Name = '${SKYRIM_PROCESS_NAME}'" -ErrorAction SilentlyContinue | Select-Object ProcessId, Name
if ($null -eq $procs) { return }
$procs | ConvertTo-Json -Compress
`;
    const raw = execFileSync('powershell.exe', ['-NoProfile', '-Command', script], {
      encoding: 'utf8',
      stdio: ['ignore', 'pipe', 'ignore'],
    }).trim();
    if (!raw) {
      return [];
    }

    const parsed = JSON.parse(raw);
    const rows = Array.isArray(parsed) ? parsed : [parsed];
    return rows
      .map((row) => ({
        pid: Number(row.ProcessId),
        name: String(row.Name ?? SKYRIM_PROCESS_NAME),
      }))
      .filter((row) => Number.isInteger(row.pid) && row.pid > 0);
  } catch {
    return [];
  }
}

function buildRuntimeSnapshot(ctx, processes) {
  const skyrimRoot = ctx?.skyrimRoot ? String(ctx.skyrimRoot) : '';
  const skyrimExePath = ctx?.skyrimExePath ? String(ctx.skyrimExePath) : '';
  const skseLoaderPath = ctx?.skseLoaderPath ? String(ctx.skseLoaderPath) : '';
  const processIds = processes.map((processInfo) => processInfo.pid);
  const processCount = processIds.length;
  const skyrimRootExists = Boolean(skyrimRoot) && fs.existsSync(skyrimRoot);
  const skyrimExeExists = Boolean(skyrimExePath) && fs.existsSync(skyrimExePath);
  const skseLoaderExists = Boolean(skseLoaderPath) && fs.existsSync(skseLoaderPath);

  let state = 'ready_to_launch';
  if (process.platform !== 'win32') {
    state = 'unsupported';
  } else if (processCount > 0) {
    state = 'running';
  } else if (!skyrimRoot) {
    state = 'skyrim_root_missing';
  } else if (!skyrimExeExists) {
    state = 'bad_skyrim_root';
  } else if (!skseLoaderExists) {
    state = 'skse_missing';
  }

  return {
    platform: process.platform,
    state,
    message: getRuntimeStateMessage({
      state,
      processCount,
      skyrimRoot,
      skseLoaderPath,
    }),
    checkedAt: new Date().toISOString(),
    running: state === 'running',
    canLaunch: state === 'ready_to_launch',
    canStop: processCount > 0,
    canOpenFolder: process.platform === 'win32' && skyrimRootExists,
    processName: SKYRIM_PROCESS_NAME,
    processCount,
    processIds,
    skyrimRoot,
    skyrimRootExists,
    skyrimExePath,
    skyrimExeExists,
    skseLoaderPath,
    skseLoaderExists,
  };
}

async function sleep(ms) {
  await new Promise((resolve) => setTimeout(resolve, ms));
}

async function waitForRuntimeSnapshot(runtime, predicate, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  let snapshot = getRuntimeStatus(runtime);

  while (Date.now() < deadline) {
    if (predicate(snapshot)) {
      return snapshot;
    }
    await sleep(PROCESS_POLL_INTERVAL_MS);
    snapshot = getRuntimeStatus(runtime);
  }

  return snapshot;
}

export function getRuntimeStatus(runtime) {
  return buildRuntimeSnapshot(runtime?.ctx ?? {}, readWindowsGameProcesses());
}

export async function launchRuntime(runtime) {
  const status = getRuntimeStatus(runtime);
  if (status.state === 'running') {
    return {
      action: 'already_running',
      status,
    };
  }
  if (status.state !== 'ready_to_launch') {
    throw createRuntimeError(status.message, runtimeErrorCodeForState(status.state), status);
  }

  await new Promise((resolve, reject) => {
    const child = spawn(status.skseLoaderPath, [], {
      cwd: status.skyrimRoot,
      detached: true,
      stdio: 'ignore',
      windowsHide: true,
    });
    child.once('error', reject);
    child.once('spawn', () => {
      child.unref();
      resolve();
    });
  });

  const nextStatus = await waitForRuntimeSnapshot(runtime, (snapshot) => snapshot.running, 2500);
  return {
    action: 'launch_requested',
    status: nextStatus,
  };
}

export async function stopRuntime(runtime) {
  const status = getRuntimeStatus(runtime);
  if (status.state === 'unsupported') {
    throw createRuntimeError(status.message, runtimeErrorCodeForState(status.state), status);
  }
  if (status.processIds.length === 0) {
    return {
      action: 'not_running',
      stoppedProcessCount: 0,
      status,
    };
  }

  for (const pid of status.processIds) {
    try {
      execFileSync('taskkill.exe', ['/PID', String(pid), '/T', '/F'], {
        stdio: ['ignore', 'ignore', 'ignore'],
      });
    } catch {
      // Best effort only. Another tool or the game itself may have already exited.
    }
  }

  const nextStatus = await waitForRuntimeSnapshot(runtime, (snapshot) => !snapshot.running, 5000);
  return {
    action: 'stop_requested',
    stoppedProcessCount: status.processIds.length,
    status: nextStatus,
  };
}

export async function openRuntimeFolder(runtime) {
  const status = getRuntimeStatus(runtime);

  if (process.platform !== 'win32') {
    throw createRuntimeError(
      'Opening the Skyrim folder is currently supported on Windows only.',
      'RUNTIME_UNSUPPORTED',
      status,
    );
  }
  if (!status.skyrimRoot) {
    throw createRuntimeError(
      'Set Skyrim Root in Settings before trying to open the game folder.',
      'RUNTIME_SKYRIM_ROOT_MISSING',
      status,
    );
  }
  if (!status.skyrimRootExists) {
    throw createRuntimeError(
      `Configured Skyrim root does not exist: ${status.skyrimRoot}`,
      'RUNTIME_FOLDER_MISSING',
      status,
    );
  }

  const escapedFolderPath = status.skyrimRoot.replace(/'/g, "''");
  const script = `$folder = '${escapedFolderPath}'; Start-Process -FilePath explorer.exe -ArgumentList @($folder)`;

  execFileSync('powershell.exe', ['-NoProfile', '-Command', script], {
    stdio: ['ignore', 'ignore', 'ignore'],
  });

  return {
    action: 'folder_open_requested',
    folderPath: status.skyrimRoot,
    status,
  };
}
