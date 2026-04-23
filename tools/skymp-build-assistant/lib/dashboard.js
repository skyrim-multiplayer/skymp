import { execFileSync } from 'child_process';
import fs from 'fs';
import path from 'path';

import express from 'express';

import { buildOrchestrator } from './build-orchestrator.js';
import { setBuildCmakeOption } from './build-profiles.js';
import { getConfigPayload, saveUserConfig } from './config.js';
import { runDoctor } from './doctor.js';
import { getModsSnapshot, reorderMods, toggleMod } from './mods-service.js';
import { getRuntimeStatus, launchRuntime, openRuntimeFolder, stopRuntime } from './runtime-supervisor.js';
import {
  dashboardUiDistDir,
  dashboardUiIndexPath,
  dashboardUiRoot,
  defaultDashboardStatePath,
  packageRoot,
} from './paths.js';
import { loadWorkspace, runStatus, runSync } from './workspace.js';

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

function getListeningProcessByPort(port) {
  if (!Number.isInteger(port) || port <= 0) {
    return null;
  }

  if (process.platform === 'win32') {
    try {
      const script = `
$conn = Get-NetTCPConnection -LocalPort ${port} -State Listen -ErrorAction SilentlyContinue | Select-Object -First 1
if ($null -eq $conn) { return }
$proc = Get-CimInstance Win32_Process -Filter "ProcessId = $($conn.OwningProcess)" | Select-Object ProcessId, Name, CommandLine
$proc | ConvertTo-Json -Compress
`;
      const raw = execFileSync('powershell', ['-NoProfile', '-Command', script], {
        encoding: 'utf8',
        stdio: ['ignore', 'pipe', 'ignore'],
      }).trim();
      if (!raw) {
        return null;
      }
      const parsed = JSON.parse(raw);
      return {
        pid: parsed.ProcessId,
        name: parsed.Name ?? '',
        commandLine: parsed.CommandLine ?? '',
      };
    } catch {
      return null;
    }
  }

  return null;
}

function looksLikeDashboardProcess(proc) {
  const commandLine = String(proc?.commandLine ?? '').toLowerCase();
  return commandLine.includes('skymp-dev') && commandLine.includes('dashboard');
}

function readDashboardState(statePath) {
  if (!fs.existsSync(statePath)) {
    return null;
  }
  const raw = fs.readFileSync(statePath, 'utf8');
  return JSON.parse(raw);
}

function writeDashboardState(statePath, state) {
  fs.writeFileSync(statePath, JSON.stringify(state, null, 2));
}

function removeDashboardState(statePath) {
  if (fs.existsSync(statePath)) {
    fs.unlinkSync(statePath);
  }
}

function summarizeRows(rows) {
  return rows.reduce(
    (summary, row) => {
      summary.total += 1;
      summary.byStatus[row.status] = (summary.byStatus[row.status] ?? 0) + 1;
      return summary;
    },
    { total: 0, byStatus: {} },
  );
}

function summarizeBuildSnapshot(snapshot) {
  const activeJob = snapshot.jobs.find((job) => job.id === snapshot.activeJobId) ?? null;
  const latestFinishedJob =
    snapshot.jobs.find((job) => ['succeeded', 'failed', 'cancelled'].includes(job.status)) ?? null;

  return {
    activeState: activeJob?.status ?? 'idle',
    activeJobId: activeJob?.id ?? null,
    queued: snapshot.queuedJobIds.length,
    trackedJobs: snapshot.jobs.length,
    lastFinishedStatus: latestFinishedJob?.status ?? null,
    lastFinishedJobId: latestFinishedJob?.id ?? null,
  };
}

function buildMissingUiHtml(uiDistDir) {
  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>SkyMP Build Assistant</title>
  <style>
    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      place-items: center;
      background: #0f172a;
      color: #e2e8f0;
      font-family: Inter, system-ui, sans-serif;
    }
    .panel {
      max-width: 720px;
      padding: 24px;
      border-radius: 20px;
      background: rgba(15, 23, 42, 0.92);
      border: 1px solid rgba(148, 163, 184, 0.2);
      box-shadow: 0 18px 40px rgba(2, 6, 23, 0.35);
    }
    code {
      display: block;
      margin-top: 12px;
      padding: 12px;
      border-radius: 12px;
      background: rgba(2, 6, 23, 0.75);
      overflow-wrap: anywhere;
    }
  </style>
</head>
<body>
  <div class="panel">
    <h1>Dashboard UI has not been built yet</h1>
    <p>The API server is running, but the frontend assets are missing.</p>
    <p>Build the dashboard UI from <code>tools/skymp-build-assistant</code> with:</p>
    <code>npm run build:ui</code>
    <p>Expected built entry:</p>
    <code>${uiDistDir}</code>
  </div>
</body>
</html>`;
}

async function attachDashboardUi(app, { dev, uiDistDir, uiIndexPath }) {
  if (dev) {
    let createViteServer;
    try {
      ({ createServer: createViteServer } = await import('vite'));
    } catch {
      throw new Error(
        'Dashboard --dev requires Vite to be installed. Run `npm install` in tools/skymp-build-assistant first.',
      );
    }

    const uiRoot = dashboardUiRoot();
    const vite = await createViteServer({
      configFile: path.join(packageRoot(), 'vite.config.js'),
      root: uiRoot,
      appType: 'custom',
      server: {
        middlewareMode: true,
      },
    });

    app.use(vite.middlewares);
    app.get('*', async (req, res, next) => {
      try {
        let template = fs.readFileSync(path.join(uiRoot, 'index.html'), 'utf8');
        template = await vite.transformIndexHtml(req.originalUrl, template);
        res.status(200).set({ 'Content-Type': 'text/html' }).end(template);
      } catch (error) {
        vite.ssrFixStacktrace?.(error);
        next(error);
      }
    });

    return vite;
  }

  if (fs.existsSync(uiIndexPath)) {
    app.use(express.static(uiDistDir));
    app.get('*', (_req, res) => {
      res.sendFile(uiIndexPath);
    });
  } else {
    app.get('*', (_req, res) => {
      res.type('html').send(buildMissingUiHtml(path.normalize(uiDistDir)));
    });
  }

  return null;
}

/**
 * Start localhost dashboard + JSON API.
 * @returns {Promise<import('http').Server>}
 */
export async function startDashboard({
  repoRoot,
  manifestPath,
  configPath,
  port = 8790,
  dev = false,
}) {
  const statePath = defaultDashboardStatePath(repoRoot);
  const existingState = readDashboardState(statePath);
  if (existingState?.pid && isPidRunning(existingState.pid)) {
    throw new Error(
      `Dashboard already running on port ${existingState.port ?? 'unknown'} (pid ${existingState.pid}). ` +
        'Use "skymp-dev dashboard-stop" first.',
    );
  }
  if (existingState) {
    removeDashboardState(statePath);
  }

  const listener = getListeningProcessByPort(port);
  if (listener) {
    if (looksLikeDashboardProcess(listener)) {
      throw new Error(
        `Dashboard already running on port ${port} (pid ${listener.pid}). ` +
          `Use "skymp-dev dashboard-stop --port ${port}" first.`,
      );
    }
    throw new Error(
      `Port ${port} is already in use by pid ${listener.pid}${listener.name ? ` (${listener.name})` : ''}.`,
    );
  }

  const uiDistDir = dashboardUiDistDir();
  const uiIndexPath = dashboardUiIndexPath();
  const app = express();
  app.use(express.json({ limit: '1mb' }));
  app.use((error, _req, res, next) => {
    if (error instanceof SyntaxError && error.type === 'entity.parse.failed') {
      res.status(400).json({ error: 'Invalid JSON body.' });
      return;
    }
    next(error);
  });

  function load() {
    const { manifestDoc, ctx, config } = loadWorkspace({
      repoRoot,
      manifestPath,
      configPath,
    });
    return { manifestDoc, ctx, config };
  }

  function buildRuntime() {
    const workspace = load();
    return {
      repoRoot,
      manifestPath,
      configPath,
      ...workspace,
    };
  }

  app.get('/api/meta', (_req, res) => {
    try {
      const { ctx } = load();
      res.json({
        appName: 'SkyMP Build Assistant',
        repoRoot,
        manifestPath,
        configPath,
        configExists: fs.existsSync(configPath),
        dashboard: {
          port,
          dev,
        },
        ui: {
          mode: dev ? 'dev' : 'built',
          built: fs.existsSync(uiIndexPath),
          distDir: uiDistDir,
        },
        ctx,
      });
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/status', (_req, res) => {
    try {
      const { manifestDoc, ctx } = load();
      const rows = runStatus(manifestDoc, ctx);
      res.json({
        repoRoot,
        ctx,
        rows,
        summary: summarizeRows(rows),
      });
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/doctor', (_req, res) => {
    try {
      const { ctx } = load();
      res.json(runDoctor(repoRoot, ctx));
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/config', (_req, res) => {
    try {
      res.json(getConfigPayload(configPath));
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.put('/api/config', (req, res) => {
    try {
      const payload = saveUserConfig(configPath, req.body?.values ?? {});
      res.json(payload);
    } catch (e) {
      if (e?.message === 'Validation failed') {
        res.status(400).json({
          error: e.message,
          details: e.details ?? [],
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/runtime', (_req, res) => {
    try {
      res.json(getRuntimeStatus(buildRuntime()));
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/runtime/launch', async (_req, res) => {
    try {
      res.json(await launchRuntime(buildRuntime()));
    } catch (e) {
      if (e?.code?.startsWith?.('RUNTIME_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          status: e.status ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/runtime/stop', async (_req, res) => {
    try {
      res.json(await stopRuntime(buildRuntime()));
    } catch (e) {
      if (e?.code?.startsWith?.('RUNTIME_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          status: e.status ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/runtime/open-folder', async (_req, res) => {
    try {
      res.json(await openRuntimeFolder(buildRuntime()));
    } catch (e) {
      if (e?.code?.startsWith?.('RUNTIME_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          status: e.status ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/mods', (_req, res) => {
    try {
      const runtime = buildRuntime();
      const runtimeStatus = getRuntimeStatus(runtime);
      res.json(getModsSnapshot(runtime, { runtimeStatus }));
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/mods/toggle', (req, res) => {
    try {
      const modId = String(req.body?.modId ?? '').trim();
      if (!modId) {
        res.status(400).json({ error: 'modId is required.' });
        return;
      }
      if (typeof req.body?.enabled !== 'boolean') {
        res.status(400).json({ error: 'enabled must be a boolean.' });
        return;
      }

      const runtime = buildRuntime();
      const runtimeStatus = getRuntimeStatus(runtime);
      const result = toggleMod(
        runtime,
        {
          modId,
          enabled: req.body.enabled,
        },
        {
          runtimeStatus,
        },
      );
      res.json(result);
    } catch (e) {
      if (e?.code === 'MOD_NOT_FOUND') {
        res.status(404).json({ error: e.message, code: e.code });
        return;
      }
      if (e?.code?.startsWith?.('MOD_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          mod: e.mod ?? null,
          snapshot: e.snapshot ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/mods/reorder', (req, res) => {
    try {
      if (!Array.isArray(req.body?.orderedModIds)) {
        res.status(400).json({ error: 'orderedModIds must be an array.' });
        return;
      }

      const runtime = buildRuntime();
      const runtimeStatus = getRuntimeStatus(runtime);
      const result = reorderMods(
        runtime,
        {
          orderedModIds: req.body.orderedModIds,
        },
        {
          runtimeStatus,
        },
      );
      res.json(result);
    } catch (e) {
      if (e?.code === 'MOD_REORDER_NOT_FOUND') {
        res.status(404).json({ error: e.message, code: e.code });
        return;
      }
      if (e?.code?.startsWith?.('MOD_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          snapshot: e.snapshot ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/sync', (req, res) => {
    try {
      const dryRun = Boolean(req.body?.dryRun);
      const onlyId = req.body?.onlyId ?? null;
      const { manifestDoc, ctx } = load();
      const out = runSync(manifestDoc, ctx, { dryRun, onlyId });
      res.json(out);
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/build/profiles', (_req, res) => {
    try {
      const runtime = buildRuntime();
      res.json({
        profiles: buildOrchestrator.listProfiles(runtime),
      });
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/build/cmake-options', (req, res) => {
    try {
      const optionKey = String(req.body?.optionKey ?? '').trim();
      if (!optionKey) {
        res.status(400).json({ error: 'optionKey is required.' });
        return;
      }
      if (typeof req.body?.enabled !== 'boolean') {
        res.status(400).json({ error: 'enabled must be a boolean.' });
        return;
      }

      const result = setBuildCmakeOption(optionKey, req.body.enabled, buildRuntime());
      res.json(result);
    } catch (e) {
      if (e?.code?.startsWith?.('BUILD_CMAKE_OPTION_')) {
        res.status(400).json({
          error: e.message,
          code: e.code,
          optionKey: e.optionKey ?? null,
        });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.get('/api/build/jobs', (_req, res) => {
    try {
      const snapshot = buildOrchestrator.getSnapshot();
      res.json({
        ...snapshot,
        summary: summarizeBuildSnapshot(snapshot),
      });
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/build/run', async (req, res) => {
    try {
      const profileId = String(req.body?.profileId ?? '').trim();
      if (!profileId) {
        res.status(400).json({ error: 'profileId is required.' });
        return;
      }

      const job = await buildOrchestrator.runProfile(profileId, buildRuntime(), {
        requestedBy: 'dashboard',
        confirmDestructive: Boolean(req.body?.confirmDestructive),
      });
      const snapshot = buildOrchestrator.getSnapshot();
      res.status(202).json({
        job,
        snapshot: {
          ...snapshot,
          summary: summarizeBuildSnapshot(snapshot),
        },
      });
    } catch (e) {
      if (e?.code === 'BUILD_PROFILE_DISABLED') {
        res.status(400).json({
          error: e.message,
          code: e.code,
          profile: e.profile ?? null,
        });
        return;
      }
      if (e?.code === 'BUILD_PROFILE_CONFIRMATION_REQUIRED') {
        res.status(400).json({
          error: e.message,
          code: e.code,
          profile: e.profile ?? null,
        });
        return;
      }
      if (e?.code === 'UNKNOWN_BUILD_PROFILE') {
        res.status(404).json({ error: e.message });
        return;
      }
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.post('/api/build/cancel', (req, res) => {
    try {
      const jobId =
        typeof req.body?.jobId === 'string' && req.body.jobId.trim() ? req.body.jobId.trim() : null;
      const result = buildOrchestrator.cancelJob(jobId);
      if (!result.cancelled) {
        res.status(404).json(result);
        return;
      }

      const snapshot = buildOrchestrator.getSnapshot();
      res.json({
        ...result,
        snapshot: {
          ...snapshot,
          summary: summarizeBuildSnapshot(snapshot),
        },
      });
    } catch (e) {
      res.status(500).json({ error: String(e.message || e) });
    }
  });

  app.use('/api', (_req, res) => {
    res.status(404).json({ error: 'Not found' });
  });

  const viteServer = await attachDashboardUi(app, {
    dev,
    uiDistDir,
    uiIndexPath,
  });

  const server = app.listen(port, () => {
    writeDashboardState(statePath, {
      pid: process.pid,
      port,
      repoRoot,
      dev,
      startedAt: new Date().toISOString(),
    });
    const mode = dev ? 'dev' : 'prod';
    console.log(`[skymp-dev] dashboard (${mode}) http://127.0.0.1:${port}/`);
  });

  let cleanedUp = false;
  const cleanup = () => {
    if (cleanedUp) {
      return;
    }
    cleanedUp = true;
    try {
      viteServer?.close?.();
    } catch {
      // Best-effort cleanup only.
    }
    try {
      const currentState = readDashboardState(statePath);
      if (currentState?.pid === process.pid) {
        removeDashboardState(statePath);
      }
    } catch {
      // Best-effort cleanup only.
    }
  };

  server.on('close', cleanup);
  process.on('SIGINT', () => {
    cleanup();
    server.close(() => process.exit(0));
  });
  process.on('SIGTERM', () => {
    cleanup();
    server.close(() => process.exit(0));
  });
  process.on('exit', cleanup);
  server.on('error', (err) => {
    cleanup();
    if (err?.code === 'EADDRINUSE') {
      console.error(`[skymp-dev] Dashboard port ${port} is already in use.`);
    } else {
      console.error(err);
    }
    process.exit(1);
  });

  return server;
}

export function stopDashboard(repoRoot, { port = 8790, force = false } = {}) {
  const statePath = defaultDashboardStatePath(repoRoot);
  const state = readDashboardState(statePath);
  let staleStatePid = null;

  if (state?.pid && isPidRunning(state.pid)) {
    process.kill(state.pid, 'SIGTERM');
    removeDashboardState(statePath);
    return {
      stopped: true,
      pid: state.pid,
      port: state.port,
      source: 'state',
    };
  }

  if (state) {
    staleStatePid = state.pid ?? null;
    removeDashboardState(statePath);
  }

  const listener = getListeningProcessByPort(port);
  if (!listener) {
    if (staleStatePid != null) {
      return { stopped: false, reason: 'stale_state', pid: staleStatePid, port };
    }
    return { stopped: false, reason: 'not_running', port };
  }

  if (!looksLikeDashboardProcess(listener) && !force) {
    return {
      stopped: false,
      reason: 'port_in_use_by_other',
      port,
      pid: listener.pid,
      processName: listener.name,
      commandLine: listener.commandLine,
    };
  }

  process.kill(listener.pid, 'SIGTERM');
  return {
    stopped: true,
    pid: listener.pid,
    port,
    source: looksLikeDashboardProcess(listener) ? 'port' : 'force',
    processName: listener.name,
  };
}
