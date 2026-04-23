import fs from 'fs';
import path from 'path';

import { buildOrchestrator } from './build-orchestrator.js';
import { startDashboard, stopDashboard } from './dashboard.js';
import { runDoctor } from './doctor.js';
import { getRuntimeStatus, launchRuntime, stopRuntime } from './runtime-supervisor.js';
import {
  defaultDashboardStatePath,
  defaultManifestPath,
  defaultUserConfigPath,
  findRepoRoot,
} from './paths.js';
import { loadWorkspace, runStatus, runSync, watchPathsForManifest } from './workspace.js';
import chokidar from 'chokidar';

function describeRequirements(profile) {
  const failing = (profile.requirements ?? []).filter((requirement) => !requirement.satisfied);
  if (failing.length === 0) {
    return 'ready';
  }
  return failing.map((requirement) => requirement.detail).join(' | ');
}

function printBuildProfiles(profiles) {
  for (const profile of profiles) {
    const status = profile.enabled ? 'ready' : 'disabled';
    console.log(`[${status}] ${profile.id} - ${profile.label}`);
    console.log(`  ${profile.description}`);
    if (profile.destructive) {
      console.log(`  destructive: ${profile.confirmMessage || 'requires --yes to run'}`);
    }
    if (profile.dependencies?.length) {
      console.log(`  depends on: ${profile.dependencies.join(', ')}`);
    }
    console.log(`  ${describeRequirements(profile)}`);
  }
}

function printBuildSnapshot(snapshot) {
  const activeJob = snapshot.jobs.find((job) => job.id === snapshot.activeJobId) ?? null;
  if (!activeJob && snapshot.queuedJobIds.length === 0 && snapshot.jobs.length === 0) {
    console.log('[skymp-dev] No build jobs are currently tracked in this process.');
    return;
  }

  if (activeJob) {
    console.log(`[running] ${activeJob.profileLabel} (${activeJob.id})`);
  } else {
    console.log('[idle] No active build job.');
  }

  if (snapshot.queuedJobIds.length > 0) {
    console.log(`Queued jobs: ${snapshot.queuedJobIds.join(', ')}`);
  }

  for (const job of snapshot.jobs.slice(0, 5)) {
    console.log(`[${job.status}] ${job.profileId} (${job.id})`);
  }
}

function printJobLogEntries(job, startIndex) {
  let nextIndex = startIndex;
  for (const entry of job.logs.slice(startIndex)) {
    if (entry.stream === 'stderr') {
      console.error(entry.message);
    } else {
      console.log(entry.message);
    }
    nextIndex += 1;
  }
  return nextIndex;
}

function sleep(ms) {
  return new Promise((resolve) => setTimeout(resolve, ms));
}

async function fetchDashboardJson(baseUrl, routePath, options = {}) {
  const response = await fetch(`${baseUrl}${routePath}`, {
    ...options,
    signal: options.signal ?? AbortSignal.timeout(1500),
    headers: {
      'Content-Type': 'application/json',
      ...(options.headers ?? {}),
    },
  });
  const text = await response.text();
  const data = text ? JSON.parse(text) : null;
  if (!response.ok) {
    const error = new Error(data?.error || `Request failed (${response.status})`);
    error.details = data?.details ?? [];
    error.code = data?.code ?? null;
    error.profile = data?.profile ?? null;
    throw error;
  }
  return data;
}

async function getDashboardBuildBaseUrl(opts, repoRoot) {
  const statePath = defaultDashboardStatePath(repoRoot);
  if (!fs.existsSync(statePath)) {
    return null;
  }

  try {
    const state = JSON.parse(fs.readFileSync(statePath, 'utf8'));
    const port = Number.isFinite(opts.port) ? opts.port : state.port;
    if (!Number.isFinite(port)) {
      return null;
    }

    const baseUrl = `http://127.0.0.1:${port}`;
    const meta = await fetchDashboardJson(baseUrl, '/api/meta');
    if (meta?.appName === 'SkyMP Build Assistant') {
      return baseUrl;
    }
  } catch {
    return null;
  }

  return null;
}

async function waitForDashboardJob(baseUrl, jobId, { json = false } = {}) {
  let printedLogs = 0;

  for (;;) {
    const snapshot = await fetchDashboardJson(baseUrl, '/api/build/jobs', {
      signal: AbortSignal.timeout(5000),
    });
    const job = snapshot.jobs.find((candidate) => candidate.id === jobId);
    if (!job) {
      throw new Error(`Dashboard no longer tracks build job ${jobId}.`);
    }

    if (!json) {
      printedLogs = printJobLogEntries(job, printedLogs);
    }

    if (['succeeded', 'failed', 'cancelled'].includes(job.status)) {
      return job;
    }

    await sleep(1500);
  }
}

function requireDestructiveConfirmation(profile, opts) {
  if (!profile?.destructive || opts.yes === true) {
    return;
  }

  const error = new Error(
    profile.confirmMessage || `Profile ${profile.id} is destructive. Re-run with --yes to confirm.`,
  );
  error.code = 'BUILD_PROFILE_CONFIRMATION_REQUIRED';
  error.profile = profile;
  throw error;
}

async function runBuildCommandViaDashboard(baseUrl, opts, runtime) {
  const action = opts._[1] ?? 'profiles';

  if (action === 'profiles' || action === 'list') {
    const payload = await fetchDashboardJson(baseUrl, '/api/build/profiles');
    if (opts.json) {
      console.log(JSON.stringify(payload, null, 2));
    } else {
      printBuildProfiles(payload.profiles ?? []);
    }
    return true;
  }

  if (action === 'jobs' || action === 'status') {
    const payload = await fetchDashboardJson(baseUrl, '/api/build/jobs');
    if (opts.json) {
      console.log(JSON.stringify(payload, null, 2));
    } else {
      printBuildSnapshot(payload);
    }
    return true;
  }

  if (action === 'cancel') {
    const payload = await fetchDashboardJson(baseUrl, '/api/build/cancel', {
      method: 'POST',
      body: JSON.stringify({ jobId: opts._[2] ?? null }),
      signal: AbortSignal.timeout(5000),
    });
    if (opts.json) {
      console.log(JSON.stringify(payload, null, 2));
    } else {
      console.log(
        `[skymp-dev] Cancelled ${payload.source === 'queue' ? 'queued' : 'active'} job ${payload.job?.id}.`,
      );
    }
    return true;
  }

  if (action === 'run') {
    const profileId = opts._[2];
    if (!profileId) {
      throw new Error('Usage: skymp-dev build run <profileId>');
    }

    const profile = buildOrchestrator.getProfile(profileId, runtime);
    requireDestructiveConfirmation(profile, opts);

    const payload = await fetchDashboardJson(baseUrl, '/api/build/run', {
      method: 'POST',
      body: JSON.stringify({ profileId, confirmDestructive: opts.yes === true }),
      signal: AbortSignal.timeout(5000),
    });

    if (!opts.json) {
      console.log(`[skymp-dev] Using dashboard build service at ${baseUrl}.`);
      console.log(`[skymp-dev] Started ${payload.job.profileLabel} (${payload.job.id}).`);
    }

    const finalJob = await waitForDashboardJob(baseUrl, payload.job.id, { json: opts.json });
    if (opts.json) {
      console.log(JSON.stringify(finalJob, null, 2));
    } else if (finalJob.status === 'succeeded') {
      console.log(`[skymp-dev] Build completed successfully (${finalJob.id}).`);
    } else if (finalJob.status === 'cancelled') {
      console.log(`[skymp-dev] Build cancelled (${finalJob.id}).`);
    } else {
      console.log(`[skymp-dev] Build failed (${finalJob.id}).`);
      if (finalJob.missingOutputs?.length) {
        for (const output of finalJob.missingOutputs) {
          console.log(`[skymp-dev] Missing output: ${output.label} -> ${output.path}`);
        }
      }
    }

    process.exitCode = finalJob.status === 'succeeded' ? 0 : 1;
    return true;
  }

  return false;
}

async function runBuildCommand(opts, runtime) {
  const dashboardBaseUrl = await getDashboardBuildBaseUrl(opts, runtime.repoRoot);
  if (dashboardBaseUrl) {
    const handled = await runBuildCommandViaDashboard(dashboardBaseUrl, opts, runtime);
    if (handled) {
      return;
    }
  }

  const action = opts._[1] ?? 'profiles';

  if (action === 'profiles' || action === 'list') {
    const profiles = buildOrchestrator.listProfiles(runtime);
    if (opts.json) {
      console.log(JSON.stringify({ profiles }, null, 2));
    } else {
      printBuildProfiles(profiles);
    }
    return;
  }

  if (action === 'jobs' || action === 'status') {
    const snapshot = buildOrchestrator.getSnapshot();
    if (opts.json) {
      console.log(JSON.stringify(snapshot, null, 2));
    } else {
      printBuildSnapshot(snapshot);
    }
    return;
  }

  if (action === 'cancel') {
    const jobId = opts._[2] ?? null;
    const result = buildOrchestrator.cancelJob(jobId);
    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
      return;
    }

    if (!result.cancelled) {
      console.log('[skymp-dev] No matching build job to cancel.');
      process.exitCode = 1;
      return;
    }

    console.log(
      `[skymp-dev] Cancelled ${result.source === 'queue' ? 'queued' : 'active'} job ${result.job?.id}.`,
    );
    return;
  }

  if (action === 'run') {
    const profileId = opts._[2];
    if (!profileId) {
      throw new Error('Usage: skymp-dev build run <profileId>');
    }

    const profile = buildOrchestrator.getProfile(profileId, runtime);
    requireDestructiveConfirmation(profile, opts);

    const job = await buildOrchestrator.runProfile(profileId, runtime, {
      requestedBy: 'cli',
      confirmDestructive: opts.yes === true,
    });

    if (opts.json) {
      const finalJob = await buildOrchestrator.waitForJob(job.id);
      console.log(JSON.stringify(finalJob, null, 2));
      process.exitCode = finalJob.status === 'succeeded' ? 0 : 1;
      return;
    }

    console.log(`[skymp-dev] Started ${job.profileLabel} (${job.id}).`);
    let printedLogs = 0;
    printedLogs = printJobLogEntries(job, printedLogs);

    const onUpdate = (snapshot) => {
      const nextJob = snapshot.jobs.find((candidate) => candidate.id === job.id);
      if (!nextJob) {
        return;
      }
      printedLogs = printJobLogEntries(nextJob, printedLogs);
    };

    buildOrchestrator.on('update', onUpdate);
    try {
      const finalJob = await buildOrchestrator.waitForJob(job.id);
      printedLogs = printJobLogEntries(finalJob, printedLogs);
      if (finalJob.status === 'succeeded') {
        console.log(`[skymp-dev] Build completed successfully (${finalJob.id}).`);
      } else if (finalJob.status === 'cancelled') {
        console.log(`[skymp-dev] Build cancelled (${finalJob.id}).`);
      } else {
        console.log(`[skymp-dev] Build failed (${finalJob.id}).`);
        if (finalJob.missingOutputs?.length) {
          for (const output of finalJob.missingOutputs) {
            console.log(`[skymp-dev] Missing output: ${output.label} -> ${output.path}`);
          }
        }
      }
      process.exitCode = finalJob.status === 'succeeded' ? 0 : 1;
    } finally {
      buildOrchestrator.off('update', onUpdate);
    }
    return;
  }

  throw new Error(`Unknown build subcommand: ${action}`);
}

function printRuntimeStatus(status) {
  console.log(`[${status.state}] ${status.message}`);
  console.log(`  skyrimRoot: ${status.skyrimRoot || 'Not configured'}`);
  console.log(`  skyrimExe: ${status.skyrimExePath || 'Not resolved'}`);
  console.log(`  skseLoader: ${status.skseLoaderPath || 'Not resolved'}`);
  console.log(`  runningProcesses: ${status.processCount}`);
}

async function runRuntimeCommand(opts, runtime) {
  const action = opts._[1] ?? 'status';

  if (action === 'status') {
    const status = getRuntimeStatus(runtime);
    if (opts.json) {
      console.log(JSON.stringify(status, null, 2));
    } else {
      printRuntimeStatus(status);
    }
    process.exitCode = status.state === 'bad_skyrim_root' ? 1 : 0;
    return;
  }

  if (action === 'launch') {
    const result = await launchRuntime(runtime);
    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
      return;
    }

    if (result.action === 'already_running') {
      console.log('[skymp-dev] Skyrim is already running.');
      return;
    }

    console.log(`[skymp-dev] Launch requested via ${result.status.skseLoaderPath}.`);
    printRuntimeStatus(result.status);
    return;
  }

  if (action === 'stop') {
    const result = await stopRuntime(runtime);
    if (opts.json) {
      console.log(JSON.stringify(result, null, 2));
      return;
    }

    if (result.action === 'not_running') {
      console.log('[skymp-dev] Skyrim is not running.');
      return;
    }

    console.log(`[skymp-dev] Force-stop requested for ${result.stoppedProcessCount} Skyrim process(es).`);
    printRuntimeStatus(result.status);
    return;
  }

  throw new Error(`Unknown runtime subcommand: ${action}`);
}

function parseArgs(argv) {
  /** @type {Record<string, any>} */
  const opts = { _: [] };
  for (let i = 2; i < argv.length; i++) {
    const a = argv[i];
    if (a === '--repo') {
      opts.repo = argv[++i];
    } else if (a === '--manifest') {
      opts.manifest = argv[++i];
    } else if (a === '--config') {
      opts.config = argv[++i];
    } else if (a === '--dry-run') {
      opts.dryRun = true;
    } else if (a === '--only') {
      opts.only = argv[++i];
    } else if (a === '--port') {
      opts.port = Number(argv[++i]);
    } else if (a === '--dev') {
      opts.dev = true;
    } else if (a === '--force') {
      opts.force = true;
    } else if (a === '--debounce') {
      opts.debounce = Number(argv[++i]);
    } else if (a === '--json') {
      opts.json = true;
    } else if (a === '--yes') {
      opts.yes = true;
    } else if (a === '--help' || a === '-h') {
      opts.help = true;
    } else if (a.startsWith('-')) {
      throw new Error(`Unknown flag: ${a}`);
    } else {
      opts._.push(a);
    }
  }
  return opts;
}

function printHelp() {
  console.log(`
skymp-dev - SkyMP local build assistant

Usage:
  skymp-dev [options] <command>

Commands:
  status       Show manifest-driven sync status (SHA256 compare)
  sync         Copy changed artifacts (like sync-dev-runtime.ps1)
  doctor       Check repo / Skyrim paths / common pitfalls
  runtime      Show, launch, and stop the Skyrim runtime via SKSE
  build        List, run, inspect, and cancel named build profiles
  watch        Watch build outputs and re-run sync
  dashboard    Local web UI + JSON API
  dashboard-stop  Stop the running dashboard server

Options:
  --repo <path>       Repository root (default: auto-detect)
  --manifest <path>   Manifest yaml (default: tools/skymp-build-assistant/manifest.default.yaml)
  --config <path>     User config yaml (default: .skymp-dev.yaml in repo root)
  --dry-run           sync only: do not write files
  --only <artifactId> sync only one manifest artifact id
  --json              status: JSON output
  --port <n>          dashboard/dashboard-stop: port (default 8790)
  --dev               dashboard: serve the UI with Vite HMR for React/CSS changes
  --force             dashboard-stop: kill the port owner even if it does not look like skymp-dev
  --debounce <ms>     watch: debounce (default 400)
  --yes               build run: confirm destructive profiles such as clean-build-dir

Build subcommands:
  skymp-dev build profiles
  skymp-dev build run <profileId>
  skymp-dev build run rebuild-all --yes
  skymp-dev build jobs
  skymp-dev build cancel [jobId]

Runtime subcommands:
  skymp-dev runtime status
  skymp-dev runtime launch
  skymp-dev runtime stop
`);
}

export async function main() {
  const opts = parseArgs(process.argv);
  if (opts.help || opts._.length === 0) {
    printHelp();
    process.exitCode = opts._.length === 0 ? 1 : 0;
    return;
  }

  const cmd = opts._[0];
  const repoRoot = opts.repo ? path.resolve(opts.repo) : findRepoRoot(process.cwd());
  const manifestPath = opts.manifest
    ? path.resolve(opts.manifest)
    : defaultManifestPath(repoRoot);
  const configPath = opts.config
    ? path.resolve(opts.config)
    : defaultUserConfigPath(repoRoot);

  const ws = loadWorkspace({ repoRoot, manifestPath, configPath });
  const runtime = {
    repoRoot,
    manifestPath,
    configPath,
    manifestDoc: ws.manifestDoc,
    ctx: ws.ctx,
    config: ws.config,
  };

  if (cmd === 'doctor') {
    const rep = runDoctor(repoRoot, ws.ctx);
    if (opts.json) {
      console.log(JSON.stringify(rep, null, 2));
    } else {
      for (const i of rep.issues) {
        console.log(`[${i.level}] ${i.code}: ${i.message}`);
      }
      process.exitCode = rep.ok ? 0 : 1;
    }
    return;
  }

  if (cmd === 'status') {
    const rows = runStatus(ws.manifestDoc, ws.ctx);
    if (opts.json) {
      console.log(JSON.stringify({ repoRoot, ctx: ws.ctx, rows }, null, 2));
    } else {
      for (const row of rows) {
        const p = row.pair;
        console.log(
          `[${row.status}] ${p.label} (${p.artifactId})\n  ${p.sourcePath}\n  -> ${p.destPath}`,
        );
      }
    }
    return;
  }

  if (cmd === 'sync') {
    const out = runSync(ws.manifestDoc, ws.ctx, {
      dryRun: opts.dryRun === true,
      onlyId: opts.only ?? null,
    });
    if (opts.json) {
      console.log(JSON.stringify(out, null, 2));
    } else {
      for (const r of out.results) {
        const p = r.pair;
        console.log(`${p.artifactId}: ${JSON.stringify(r.result)}`);
      }
      for (const s of out.stale) {
        console.log(`[stale ${s.rule}] removed ${s.actions.length} file(s)`);
      }
    }
    return;
  }

  if (cmd === 'build') {
    await runBuildCommand(opts, runtime);
    return;
  }

  if (cmd === 'runtime') {
    await runRuntimeCommand(opts, runtime);
    return;
  }

  if (cmd === 'watch') {
    const debounceMs = Number.isFinite(opts.debounce) ? opts.debounce : 400;
    const paths = watchPathsForManifest(ws.manifestDoc, ws.ctx);
    if (paths.length === 0) {
      console.error('[skymp-dev] Nothing to watch - run a CMake build first so outputs exist.');
      process.exitCode = 1;
      return;
    }
    console.log('[skymp-dev] Watching:', paths.join(', '));
    let t = /** @type {ReturnType<typeof setTimeout> | null} */ (null);
    const run = () => {
      console.log('[skymp-dev] Syncing...');
      const out = runSync(ws.manifestDoc, ws.ctx, { dryRun: false });
      const changed = out.results.filter((x) => x.result?.synced || x.result?.copiedFiles > 0);
      console.log(`[skymp-dev] Done (${changed.length} pair(s) touched).`);
    };
    const watcher = chokidar.watch(paths, { ignoreInitial: true });
    watcher.on('all', () => {
      if (t) {
        clearTimeout(t);
      }
      t = setTimeout(run, debounceMs);
    });
    return;
  }

  if (cmd === 'dashboard') {
    const port = Number.isFinite(opts.port) ? opts.port : 8790;
    await startDashboard({
      repoRoot,
      manifestPath,
      configPath,
      port,
      dev: opts.dev === true,
    });
    return;
  }

  if (cmd === 'dashboard-stop') {
    const result = stopDashboard(repoRoot, {
      port: Number.isFinite(opts.port) ? opts.port : 8790,
      force: opts.force === true,
    });
    if (result.stopped) {
      console.log(
        `[skymp-dev] Stopped dashboard on port ${result.port ?? 'unknown'} (pid ${result.pid}).`,
      );
    } else if (result.reason === 'port_in_use_by_other') {
      console.log(
        `[skymp-dev] Port ${result.port} is in use by pid ${result.pid}` +
          `${result.processName ? ` (${result.processName})` : ''}. ` +
          'It does not look like skymp-dev dashboard, so it was not stopped.',
      );
    } else if (result.reason === 'stale_state') {
      console.log(
        `[skymp-dev] Removed stale dashboard state for pid ${result.pid}. No running server was found.`,
      );
    } else {
      console.log('[skymp-dev] Dashboard is not running.');
    }
    return;
  }

  throw new Error(`Unknown command: ${cmd}`);
}
