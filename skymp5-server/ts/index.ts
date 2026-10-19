import * as chokidar from 'chokidar';
import * as crypto from 'crypto';
import * as fs from 'fs';

import * as os from 'os';
import * as path from 'path';
// @ts-ignore
import * as sourceMapSupport from 'source-map-support';
import { spawn, spawnSync, type ChildProcessWithoutNullStreams } from 'child_process';
import { EventEmitter } from 'events';
import { pid } from 'process';
import * as readline from 'readline';

import * as manifestGen from './manifestGen';
import {
  PluginStateEntry,
  PluginDiscoveryState,
  PLUGINS_DIR,
  PLUGIN_DISCOVERY_STATE_PATH,
  readDiscoveryState,
  writeDiscoveryState,
} from './pluginDiscoveryState';
import * as scampNative from './scampNative';
import * as ui from './ui';
import { createScampServer } from './scampNative';
import { Settings } from './settings';
import { DiscordBanSystem } from './systems/discordBanSystem';
import { Login } from './systems/login';
import { MasterApiBalanceSystem } from './systems/masterApiBalanceSystem';
import { MasterClient } from './systems/masterClient';
import {
  MetricsSystem,
  tickDurationHistogram,
  tickDurationSummary,
} from './systems/metricsSystem';
import { EffectsLearningSystem } from './systems/effectsLearningSystem';
import { EnchantmentsSystem } from './systems/enchantmentsSystem';
import { FavoritesSystem } from './systems/favoritesSystem';
import { MarkerSystem } from './systems/markerSystem';
import { MovementDebugLogSystem } from './systems/movementDebugLogSystem';
import { TimeSystem } from './systems/timeSystem';
import { WeatherSystem } from './systems/weatherSystem';
import { Spawn } from './systems/spawn';
import { System } from './systems/system';
import {
  initializeVoiceActivityManager,
  type VoiceActivityState,
} from './voiceActivityApi';
import { createYacaTeamSpeakAdapter } from './adapters/YacaTeamSpeakAdapter';

sourceMapSupport.install({
  retrieveSourceMap: function (source: string) {
    if (source.endsWith('skymp5-server.js')) {
      return {
        url: 'original.js',
        map: require('fs').readFileSync(
          'dist_back/skymp5-server.js.map',
          'utf8',
        ),
      };
    }
    return null;
  },
});

const gamemodeCache = new Map<string, string>();

function requireTemp(module: string) {
  // https://blog.mastykarz.nl/create-temp-directory-app-node-js/
  let tmpDir;
  const appPrefix = 'skymp5-server';
  try {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), appPrefix));

    const contents = fs.readFileSync(module, 'utf8');
    const tempPath = path.join(
      tmpDir,
      Math.random() + '-' + Date.now() + '.js',
    );
    fs.writeFileSync(tempPath, contents);

    require(tempPath);
  } catch (e) {
    console.error(e.stack);
  } finally {
    try {
      if (tmpDir) {
        fs.rmSync(tmpDir, { recursive: true });
      }
    } catch (e) {
      console.error(
        `An error has occurred while removing the temp folder at ${tmpDir}. Please remove it manually. Error: ${e}`,
      );
    }
  }
}

function requireUncached(
  module: string,
  clear: () => void,
  server: scampNative.ScampServer,
): void {
  let gamemodeContents = fs.readFileSync(require.resolve(module), 'utf8');

  // Reload gamemode.js only if there are real changes
  const gamemodeContentsOld = gamemodeCache.get(module);
  if (gamemodeContentsOld !== gamemodeContents) {
    gamemodeCache.set(module, gamemodeContents);

    while (1) {
      try {
        clear();

        // In native module we now register mp-api methods into the ScampServer class
        // This workaround allows code that is bound to global 'mp' object to run
        // @ts-ignore
        globalThis.mp = globalThis.mp || server;

        requireTemp(module);
        return;
      } catch (e) {
        if (`${e}`.indexOf("'JsRun' returned error 0x30002") === -1) {
          throw e;
        } else {
          console.log('Bad syntax, ignoring');
          return;
        }
      }
    }
  }
}

const setupStreams = (scampNative: any, logDirPath: string) => {
  fs.mkdirSync(logDirPath, { recursive: true });

  cleanupServerLogs(logDirPath, 14, 30);

  const timestamp = new Date().toISOString().replace(/[.:]/g, '-');
  const serverLogPath = path.join(logDirPath, `server-${timestamp}.log`);
  const logFileStream = fs.createWriteStream(serverLogPath, { flags: 'a' });
  const originalStdoutWrite = process.stdout.write.bind(process.stdout);
  const originalStderrWrite = process.stderr.write.bind(process.stderr);

  originalStdoutWrite(`Server log file: ${serverLogPath}\n` as any);

  class LogsStream {
    constructor(
      private logLevel: string,
      private originalWrite: (
        chunk: string,
        encoding?: BufferEncoding,
        cb?: (error?: Error | null) => void,
      ) => boolean,
    ) {}

    write(chunk: Buffer, encoding: string, callback: () => void) {
      // @ts-ignore
      const str = chunk.toString(encoding);
      if (str.trim().length > 0) {
        const lines = str.split(/\r?\n/).filter((line) => line.length > 0);
        for (const line of lines) {
          logFileStream.write(
            `[${new Date().toISOString()}] [${this.logLevel}] ${line}\n`,
          );
        }
        ui.pushServerLogChunk(
          this.logLevel === 'error' ? 'error' : 'info',
          str,
        );
        scampNative.writeLogs(this.logLevel, str);
      }

      this.originalWrite(str, encoding as BufferEncoding);
      callback();
    }
  }

  const infoStream = new LogsStream('info', originalStdoutWrite as any);
  const errorStream = new LogsStream('error', originalStderrWrite as any);
  // @ts-ignore
  process.stdout.write = (
    chunk: Buffer,
    encoding: string,
    callback: () => void,
  ) => {
    infoStream.write(chunk, encoding, callback);
  };
  // @ts-ignore
  process.stderr.write = (
    chunk: Buffer,
    encoding: string,
    callback: () => void,
  ) => {
    errorStream.write(chunk, encoding, callback);
  };
};

const cleanupServerLogs = (
  logDirPath: string,
  keepDays: number,
  maxFiles: number,
) => {
  try {
    const now = Date.now();
    const keepMs = keepDays * 24 * 60 * 60 * 1000;
    const files = fs
      .readdirSync(logDirPath)
      .filter((name) => /^server-.*\.log$/.test(name))
      .map((name) => {
        const fullPath = path.join(logDirPath, name);
        const stat = fs.statSync(fullPath);
        return { name, fullPath, mtimeMs: stat.mtimeMs };
      })
      .sort((left, right) => right.mtimeMs - left.mtimeMs);

    for (const entry of files) {
      if (now - entry.mtimeMs > keepMs) {
        fs.rmSync(entry.fullPath, { force: true });
      }
    }

    const remaining = fs
      .readdirSync(logDirPath)
      .filter((name) => /^server-.*\.log$/.test(name))
      .map((name) => {
        const fullPath = path.join(logDirPath, name);
        const stat = fs.statSync(fullPath);
        return { fullPath, mtimeMs: stat.mtimeMs };
      })
      .sort((left, right) => right.mtimeMs - left.mtimeMs);

    for (const entry of remaining.slice(maxFiles)) {
      fs.rmSync(entry.fullPath, { force: true });
    }
  } catch (e) {
    // Keep server startup robust even if log cleanup fails
    console.warn('[logs] failed to cleanup old server logs', e);
  }
};

type PluginKind = 'gamemode' | 'process';

interface PluginManifest {
  name: string;
  version: string;
  kind: PluginKind;
  displayName: string;
  description: string;
  main: string;
  optional?: boolean;
  startupDefault?: boolean;
  command?: string;
  args?: string[];
}

interface DiscoveredPlugin {
  manifestPath: string;
  pluginDir: string;
  manifest: PluginManifest;
  fingerprint: string;
}

interface PluginSetupOptions {
  mode: 'prompt' | 'safe';
  loadOrder: string[];
  abortOnPluginError: boolean;
  processCommandAllowlist: string[] | null;
}

const pluginsLog = (...args: unknown[]) => console.log('[plugins]', ...args);
const pluginsWarn = (...args: unknown[]) => console.warn('[plugins]', ...args);
const pluginsError = (...args: unknown[]) => console.error('[plugins]', ...args);

const spawnedPluginProcesses: ChildProcessWithoutNullStreams[] = [];
let shutdownHooksInstalled = false;

const toAbsolutePath = (p: string): string => {
  if (path.isAbsolute(p)) {
    return path.normalize(p);
  }
  return path.normalize(path.resolve('', p));
};

const getAlternativeNtfsAliasPath = (absolutePath: string): string | null => {
  if (process.platform !== 'win32') {
    return null;
  }

  const normalized = path.normalize(absolutePath);
  const lower = normalized.toLowerCase();
  const dRoot = 'd:\\github\\skymp';
  const cRoot = 'c:\\github\\skymp';

  const isAtOrInside = (value: string, root: string) =>
    value === root || value.startsWith(root + '\\');

  if (isAtOrInside(lower, dRoot)) {
    return 'C:' + normalized.slice(2);
  }
  if (isAtOrInside(lower, cRoot)) {
    return 'D:' + normalized.slice(2);
  }
  return null;
};

const resolveExistingPath = (p: string): string => {
  const absolute = toAbsolutePath(p);
  const candidates = [absolute];
  const aliasCandidate = getAlternativeNtfsAliasPath(absolute);
  if (aliasCandidate) {
    candidates.push(aliasCandidate);
  }

  for (const candidate of candidates) {
    if (!fs.existsSync(candidate)) {
      continue;
    }

    try {
      return fs.realpathSync.native(candidate);
    } catch {
      return candidate;
    }
  }

  return absolute;
};

const sha256 = (content: string): string => {
  const hash = crypto.createHash('sha256');
  hash.update(content);
  return hash.digest('hex');
};

const ensureMainPath = (manifest: Partial<PluginManifest>): string => {
  if (typeof manifest.main === 'string' && manifest.main.trim().length > 0) {
    return manifest.main.trim();
  }
  return 'index.cjs';
};

const SAFE_PLUGIN_NAME_RE = /^[a-zA-Z0-9_-]{1,64}$/;
const SAFE_VERSION_RE = /^\d/;

const isRelativePathSafe = (p: string): boolean => {
  if (p.includes('\0')) return false;
  const normalized = path.normalize(p);
  // disallow absolute paths and traversal out of the plugin directory
  return !path.isAbsolute(normalized) && !normalized.startsWith('..');
};

const readPluginManifest = (manifestPath: string): PluginManifest | null => {
  let raw: string;
  try {
    raw = fs.readFileSync(manifestPath, 'utf8');
  } catch (e) {
    pluginsError(`Cannot read plugin manifest at "${manifestPath}":`, e);
    return null;
  }

  if (raw.length > 64 * 1024) {
    pluginsWarn(`Skipping "${manifestPath}": manifest exceeds 64 KiB`);
    return null;
  }

  let parsed: Partial<PluginManifest>;
  try {
    parsed = JSON.parse(raw) as Partial<PluginManifest>;
  } catch (e) {
    pluginsError(`Manifest is not valid JSON at "${manifestPath}":`, e);
    return null;
  }

  if (typeof parsed !== 'object' || parsed === null || Array.isArray(parsed)) {
    pluginsWarn(`Skipping "${manifestPath}": manifest root must be an object`);
    return null;
  }

  // ── required string fields ──────────────────────────────────────────────
  const requiredStringFields = [
    'name',
    'version',
    'kind',
    'displayName',
    'description',
  ] as const;

  for (const field of requiredStringFields) {
    const value = parsed[field];
    if (typeof value !== 'string' || value.trim().length === 0) {
      pluginsWarn(`Skipping "${manifestPath}": field "${field}" is required`);
      return null;
    }
  }

  // ── semantic constraints ────────────────────────────────────────────────
  const name = parsed.name!.trim();
  if (!SAFE_PLUGIN_NAME_RE.test(name)) {
    pluginsWarn(
      `Skipping "${manifestPath}": "name" must match /^[a-zA-Z0-9_-]{1,64}$/`,
    );
    return null;
  }

  const version = parsed.version!.trim();
  if (!SAFE_VERSION_RE.test(version)) {
    pluginsWarn(
      `Skipping "${manifestPath}": "version" must start with a digit (e.g. "1.0.0")`,
    );
    return null;
  }

  const displayName = parsed.displayName!.trim();
  if (displayName.length > 100) {
    pluginsWarn(`Skipping "${manifestPath}": "displayName" must be <= 100 chars`);
    return null;
  }

  const description = parsed.description!.trim();
  if (description.length > 512) {
    pluginsWarn(`Skipping "${manifestPath}": "description" must be <= 512 chars`);
    return null;
  }

  if (parsed.kind !== 'gamemode' && parsed.kind !== 'process') {
    pluginsWarn(
      `Skipping "${manifestPath}": field "kind" must be "gamemode" or "process"`,
    );
    return null;
  }

  const main = ensureMainPath(parsed);
  if (!isRelativePathSafe(main)) {
    pluginsWarn(
      `Skipping "${manifestPath}": "main" must be a safe relative path (no traversal)`,
    );
    return null;
  }

  if (parsed.kind === 'process') {
    if (typeof parsed.command !== 'string' || parsed.command.trim().length === 0) {
      pluginsWarn(
        `Skipping "${manifestPath}": process plugin requires non-empty string field "command"`,
      );
      return null;
    }
  }

  if (
    parsed.args !== undefined &&
    (!Array.isArray(parsed.args) ||
      parsed.args.some((arg) => typeof arg !== 'string') ||
      parsed.args.length > 128)
  ) {
    pluginsWarn(
      `Skipping "${manifestPath}": field "args" must be a string array with <= 128 entries`,
    );
    return null;
  }

  if (
    parsed.optional !== undefined &&
    typeof parsed.optional !== 'boolean'
  ) {
    pluginsWarn(`Skipping "${manifestPath}": field "optional" must be boolean`);
    return null;
  }

  return {
    name,
    version,
    kind: parsed.kind,
    displayName,
    description,
    main,
    optional: parsed.optional === true,
    startupDefault: parsed.startupDefault === true,
    command:
      typeof parsed.command === 'string' ? parsed.command.trim() : undefined,
    args: parsed.args,
  };
};

const fingerprintPlugin = (
  manifestPath: string,
  pluginDir: string,
  manifest: PluginManifest,
): string => {
  const parts: string[] = [];
  const manifestRaw = fs.readFileSync(manifestPath, 'utf8');
  parts.push(`manifest:${manifestRaw}`);

  const mainAbsolute = path.resolve(pluginDir, manifest.main);
  if (fs.existsSync(mainAbsolute)) {
    const stat = fs.statSync(mainAbsolute);
    parts.push(`main:${manifest.main}:${stat.size}:${stat.mtimeMs}`);
  } else {
    parts.push(`main:${manifest.main}:missing`);
  }

  if (manifest.kind === 'process') {
    parts.push(`command:${manifest.command || ''}`);
    parts.push(`args:${JSON.stringify(manifest.args || [])}`);
  }

  return sha256(parts.join('\n'));
};

const discoverPlugins = (): DiscoveredPlugin[] => {
  if (!fs.existsSync(PLUGINS_DIR)) {
    pluginsLog(`No plugins directory found at "${PLUGINS_DIR}"`);
    return [];
  }

  const entries = fs
    .readdirSync(PLUGINS_DIR, { withFileTypes: true })
    .filter((entry) => entry.isDirectory());

  const plugins: DiscoveredPlugin[] = [];
  const byName = new Set<string>();

  for (const entry of entries) {
    const pluginDir = path.join(PLUGINS_DIR, entry.name);
    const manifestPath = path.join(pluginDir, 'plugin.json');

    if (!fs.existsSync(manifestPath)) {
      continue;
    }

    const manifest = readPluginManifest(manifestPath);
    if (!manifest) {
      continue;
    }

    if (byName.has(manifest.name)) {
      pluginsWarn(
        `Duplicate plugin name "${manifest.name}" in "${pluginDir}"; skipping`,
      );
      continue;
    }
    byName.add(manifest.name);

    plugins.push({
      manifestPath,
      pluginDir,
      manifest,
      fingerprint: fingerprintPlugin(manifestPath, pluginDir, manifest),
    });
  }

  return plugins;
};

const askToEnablePlugin = async (plugin: DiscoveredPlugin): Promise<boolean> => {
  if (!process.stdin.isTTY || !process.stdout.isTTY) {
    pluginsLog(
      `Non-interactive mode detected, leaving plugin "${plugin.manifest.name}" disabled`,
    );
    return false;
  }

  const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout,
  });

  const answer = await new Promise<string>((resolve) => {
    rl.question(
      `[plugins] Enable startup for plugin "${plugin.manifest.displayName}" (${plugin.manifest.name}@${plugin.manifest.version})? ${plugin.manifest.description} [y/N]: `,
      resolve,
    );
  });

  rl.close();

  const normalized = answer.trim().toLowerCase();
  return normalized === 'y' || normalized === 'yes';
};

const buildPluginSortOrder = (
  plugins: DiscoveredPlugin[],
  preferredOrder: string[],
): DiscoveredPlugin[] => {
  const orderIndex = new Map<string, number>();
  preferredOrder.forEach((name, index) => orderIndex.set(name, index));

  return plugins.slice().sort((a, b) => {
    const aIndex = orderIndex.get(a.manifest.name);
    const bIndex = orderIndex.get(b.manifest.name);

    if (aIndex !== undefined && bIndex !== undefined) {
      return aIndex - bIndex;
    }
    if (aIndex !== undefined) {
      return -1;
    }
    if (bIndex !== undefined) {
      return 1;
    }

    return a.manifest.name.localeCompare(b.manifest.name);
  });
};

const terminatePluginProcessTree = (child: ChildProcessWithoutNullStreams) => {
  if (!child.pid || child.killed) {
    return;
  }

  if (process.platform === 'win32') {
    try {
      spawnSync('taskkill', ['/pid', String(child.pid), '/t', '/f'], {
        stdio: 'ignore',
        windowsHide: true,
      });
    } catch (e) {
      pluginsWarn(`Failed to terminate plugin process ${child.pid}:`, e);
    }
    return;
  }

  try {
    child.kill('SIGTERM');
  } catch (e) {
    pluginsWarn(`Failed to SIGTERM plugin process ${child.pid}:`, e);
  }
};

const installShutdownHooks = () => {
  if (shutdownHooksInstalled) {
    return;
  }
  shutdownHooksInstalled = true;

  const stopAllPluginProcesses = () => {
    for (const child of spawnedPluginProcesses) {
      terminatePluginProcessTree(child);
    }
  };

  process.on('SIGINT', stopAllPluginProcesses);
  process.on('SIGTERM', stopAllPluginProcesses);
  process.on('exit', stopAllPluginProcesses);
};

const normalizeCommandToken = (value: string): string =>
  process.platform === 'win32' ? value.toLowerCase() : value;

const isProcessPluginCommandAllowed = (
  command: string,
  processCommandAllowlist: string[] | null,
): boolean => {
  // Backward-compatible default: no allowlist means no restriction.
  if (!processCommandAllowlist) {
    return true;
  }

  const normalizedAllowlist = new Set(
    processCommandAllowlist.map((item) => normalizeCommandToken(item.trim())),
  );
  const commandTrimmed = command.trim();
  const commandBase = path.basename(commandTrimmed);
  const normalizedCommand = normalizeCommandToken(commandTrimmed);
  const normalizedBase = normalizeCommandToken(commandBase);

  return (
    normalizedAllowlist.has(normalizedCommand) ||
    normalizedAllowlist.has(normalizedBase)
  );
};

const runGamemodePlugin = (
  plugin: DiscoveredPlugin,
  server: scampNative.ScampServer,
) => {
  const mainPath = resolveExistingPath(path.resolve(plugin.pluginDir, plugin.manifest.main));
  if (!fs.existsSync(mainPath)) {
    throw new Error(`entry file does not exist: ${mainPath}`);
  }

  // @ts-ignore
  globalThis.mp = globalThis.mp || server;
  requireTemp(mainPath);
};

const runProcessPlugin = (
  plugin: DiscoveredPlugin,
  processCommandAllowlist: string[] | null,
) => {
  const command = plugin.manifest.command;
  if (!command) {
    throw new Error('process plugin has no command');
  }

  if (!isProcessPluginCommandAllowed(command, processCommandAllowlist)) {
    const allowlistInfo =
      processCommandAllowlist && processCommandAllowlist.length > 0
        ? processCommandAllowlist.join(', ')
        : '(empty allowlist)';
    throw new Error(
      `process plugin command "${command}" is not allowed by pluginDiscovery.processCommandAllowlist=${allowlistInfo}`,
    );
  }

  const child = spawn(command, plugin.manifest.args || [], {
    cwd: plugin.pluginDir,
    windowsHide: true,
    stdio: 'pipe',
  });

  ui.updatePluginRuntimeStatus(plugin.manifest.name, 'running', {
    pid: child.pid || undefined,
  });

  child.stdout.on('data', (chunk) => {
    process.stdout.write(`[plugin:${plugin.manifest.name}] ${chunk}`);
  });
  child.stderr.on('data', (chunk) => {
    process.stderr.write(`[plugin:${plugin.manifest.name}] ${chunk}`);
  });
  child.on('exit', (code, signal) => {
    const stoppedStatus = code === 0 ? 'stopped' : 'failed';
    ui.updatePluginRuntimeStatus(plugin.manifest.name, stoppedStatus, {
      pid: child.pid || undefined,
      message: `process exited with code=${code} signal=${signal}`,
    });
    pluginsWarn(
      `process plugin "${plugin.manifest.name}" exited with code=${code} signal=${signal}`,
    );
  });
  child.on('error', (e) => {
    ui.updatePluginRuntimeStatus(plugin.manifest.name, 'failed', {
      pid: child.pid || undefined,
      message: e instanceof Error ? e.message : String(e),
    });
    pluginsError(`process plugin "${plugin.manifest.name}" failed:`, e);
  });

  spawnedPluginProcesses.push(child);
  pluginsLog(
    `started process plugin "${plugin.manifest.name}" with pid ${child.pid}`,
  );
};

const bootstrapPlugins = async (
  server: scampNative.ScampServer,
  options: PluginSetupOptions,
): Promise<void> => {
  const discovered = discoverPlugins();
  if (discovered.length === 0) {
    return;
  }

  installShutdownHooks();

  const nowIso = new Date().toISOString();
  const previousState = readDiscoveryState();
  const nextState: PluginDiscoveryState = {
    version: 1,
    plugins: {},
  };

  const sorted = buildPluginSortOrder(discovered, options.loadOrder);

  for (const plugin of sorted) {
    const previous = previousState.plugins[plugin.manifest.name];
    const isKnown = previous !== undefined;
    const isUpdated =
      isKnown &&
      (previous.fingerprint !== plugin.fingerprint ||
        previous.version !== plugin.manifest.version);

    let startupEnabled = false;
    if (isKnown) {
      startupEnabled = previous.startupEnabled;
    } else if (plugin.manifest.startupDefault === true) {
      startupEnabled = false;
      pluginsWarn(
        `plugin "${plugin.manifest.name}" requests startupDefault=true, but new plugins are never auto-enabled`,
      );
    }

    if (!isKnown) {
      pluginsLog(
        `discovered new plugin "${plugin.manifest.name}" (${plugin.manifest.version})`,
      );
      if (options.mode === 'prompt') {
        startupEnabled = await askToEnablePlugin(plugin);
      }
    } else if (isUpdated) {
      pluginsLog(
        `plugin "${plugin.manifest.name}" updated (${previous.version} -> ${plugin.manifest.version}), startupEnabled=${startupEnabled}`,
      );
    } else {
      pluginsLog(
        `plugin "${plugin.manifest.name}" already known, startupEnabled=${startupEnabled}`,
      );
    }

    nextState.plugins[plugin.manifest.name] = {
      pluginPath: path.relative(process.cwd(), plugin.pluginDir),
      fingerprint: plugin.fingerprint,
      version: plugin.manifest.version,
      startupEnabled,
      discoveredAt: previous?.discoveredAt || nowIso,
      updatedAt: nowIso,
    };

    if (!startupEnabled) {
      ui.updatePluginRuntimeStatus(plugin.manifest.name, 'stopped', {
        message: 'startupEnabled=false',
      });
      continue;
    }

    try {
      ui.updatePluginRuntimeStatus(plugin.manifest.name, 'starting');
      if (plugin.manifest.kind === 'gamemode') {
        runGamemodePlugin(plugin, server);
        ui.updatePluginRuntimeStatus(plugin.manifest.name, 'running');
        pluginsLog(`loaded gamemode plugin "${plugin.manifest.name}"`);
      } else {
        runProcessPlugin(plugin, options.processCommandAllowlist);
      }
    } catch (e) {
      const msg =
        e instanceof Error ? `${e.message}\n${e.stack || ''}` : String(e);
      ui.updatePluginRuntimeStatus(plugin.manifest.name, 'failed', {
        message: msg,
      });
      if (plugin.manifest.optional) {
        pluginsWarn(
          `optional plugin "${plugin.manifest.name}" failed to start: ${msg}`,
        );
      } else {
        pluginsError(`plugin "${plugin.manifest.name}" failed to start: ${msg}`);
        if (options.abortOnPluginError) {
          pluginsError(
            `aborting server start because pluginDiscovery.abortOnPluginError=true`,
          );
          writeDiscoveryState(nextState);
          process.exit(-1);
        }
      }
    }
  }

  const removed = Object.keys(previousState.plugins).filter(
    (name) => !(name in nextState.plugins),
  );
  for (const pluginName of removed) {
    pluginsLog(`plugin removed from disk: "${pluginName}"`);
  }

  writeDiscoveryState(nextState);
};

const setupGamemode = async (
  server: any,
  gamemodePath: string,
  pluginOptions: PluginSetupOptions,
) => {
  const clear = () => server.clear();

  const absoluteGamemodePath = resolveExistingPath(gamemodePath);
  console.log(`Gamemode path is "${absoluteGamemodePath}"`);

  if (!fs.existsSync(absoluteGamemodePath)) {
    console.log(
      `Error during loading a gamemode from "${absoluteGamemodePath}" - file or directory does not exist`,
    );
    return;
  }

  try {
    requireUncached(absoluteGamemodePath, clear, server);
  } catch (e) {
    console.error(e);
  }

  await bootstrapPlugins(server, pluginOptions);

  const watcher = chokidar.watch(absoluteGamemodePath, {
    ignored: /^\./,
    persistent: true,
    awaitWriteFinish: true,
  });

  const numReloads = { n: 0 };

  const reloadGamemode = () => {
    try {
      requireUncached(absoluteGamemodePath, clear, server);
      numReloads.n++;
    } catch (e) {
      console.error(e);
    }
  };

  const reloadGamemodeTimeout = function () {
    const n = numReloads.n;
    setTimeout(() => (n === numReloads.n ? reloadGamemode() : undefined), 1000);
  };

  watcher.on('add', reloadGamemodeTimeout);
  watcher.on('addDir', reloadGamemodeTimeout);
  watcher.on('change', reloadGamemodeTimeout);
  watcher.on('unlink', reloadGamemodeTimeout);
  watcher.on('error', function (error) {
    console.error('Error happened in chokidar watch', error);
  });
};

const main = async () => {
  const settingsObject = await Settings.get();
  const {
    port,
    master,
    maxPlayers,
    name,
    masterKey,
    offlineMode,
    gamemodePath,
  } = settingsObject;

  const trimOrNull = (value: unknown): string | null => {
    if (typeof value !== 'string') {
      return null;
    }
    const trimmed = value.trim();
    return trimmed.length > 0 ? trimmed : null;
  };

  const isInvalidPublicHost = (value: string | null): boolean => {
    if (!value) {
      return true;
    }
    const normalized = value.toLowerCase();
    return (
      normalized === '0.0.0.0' || normalized === '::' || normalized === '::0'
    );
  };

  const heartbeatIp =
    trimOrNull(settingsObject.publicHost) ||
    trimOrNull(settingsObject.externalHost) ||
    (!isInvalidPublicHost(trimOrNull(settingsObject.listenHost))
      ? trimOrNull(settingsObject.listenHost)
      : null) ||
    '127.0.0.1';

  const heartbeatGamemode =
    trimOrNull(settingsObject.gamemode) ||
    trimOrNull((settingsObject.allSettings as any)?.gamemode) ||
    trimOrNull((settingsObject.allSettings as any)?.serverName);

  const heartbeatCountryCode =
    trimOrNull(settingsObject.countryCode) ||
    trimOrNull((settingsObject.allSettings as any)?.countryCode);

  const heartbeatServerUid =
    trimOrNull(settingsObject.serverUid) ||
    trimOrNull((settingsObject.allSettings as any)?.server_uid);

  const heartbeatIntervalMsRaw = Number(
    (settingsObject.allSettings as any)?.masterHeartbeatIntervalMs ??
      settingsObject.masterHeartbeatIntervalMs,
  );
  const heartbeatIntervalMs = Number.isFinite(heartbeatIntervalMsRaw)
    ? heartbeatIntervalMsRaw
    : 15000;

  const log = console.log;
  const systems = new Array<System>();
  systems.push(
    new MetricsSystem(),
    new MasterClient(
      log,
      port,
      heartbeatIp,
      master,
      maxPlayers,
      name,
      masterKey,
      heartbeatIntervalMs,
      offlineMode,
      heartbeatGamemode || undefined,
      heartbeatCountryCode || undefined,
      heartbeatServerUid || undefined,
    ),
    new Spawn(log),
    new Login(log, maxPlayers, master, port, masterKey, offlineMode),
    new DiscordBanSystem(),
    new MasterApiBalanceSystem(
      log,
      maxPlayers,
      master,
      port,
      masterKey,
      offlineMode,
    ),
    new MarkerSystem(log),
    new TimeSystem(log, settingsObject.timeScale),
    new WeatherSystem(log),
    new EffectsLearningSystem(log),
    new EnchantmentsSystem(log),
    new FavoritesSystem(log),
    new MovementDebugLogSystem(log),
  );

  setupStreams(scampNative.getScampNative(), path.resolve('logs'));

  try {
    manifestGen.generateManifest(settingsObject);
  } catch (e) {
    console.error(
      '[manifestGen] Failed to generate manifest (server will continue):',
      e instanceof Error ? e.message : e,
    );
    console.error(
      '[manifestGen] Make sure \'dataDir\' in server-settings.json points to your Skyrim Data folder',
    );
  }
  ui.main(settingsObject);

  let server: any;
  const connectedUsers = new Set<number>();

  try {
    server = createScampServer(settingsObject.allSettings);
    ui.setServer(server);
  } catch (e) {
    console.error(e);
    console.error(`Stopping the server due to the previous error`);
    process.exit(-1);
  }
  const ctx = { svr: server, gm: new EventEmitter() };

  console.log(`Current process ID is ${pid}`);

  (async () => {
    while (1) {
      const endTimerHistogram = tickDurationHistogram.startTimer();
      const endTimerSummary = tickDurationSummary.startTimer();
      try {
        server.tick();
        await new Promise((r) => setTimeout(r, 1));
      } catch (e) {
        console.error(`in server.tick:\n${e.stack}`);
      } finally {
        endTimerHistogram();
        endTimerSummary();
      }
    }
  })();

  for (const system of systems) {
    if (system.initAsync) {
      await system.initAsync(ctx);
    }
    log(`Initialized ${system.systemName}`);
    if (system.updateAsync)
      (async () => {
        while (1) {
          await new Promise((r) => setTimeout(r, 1));
          try {
            await system.updateAsync(ctx);
          } catch (e) {
            console.error(e);
          }
        }
      })();
  }

  server.on('connect', (userId: number) => {
    log('connect', userId);
    connectedUsers.add(userId);
    const showCharacterNicknames = Boolean(
      (settingsObject.allSettings as Record<string, unknown> | null)
        ?.showCharacterNicknames ?? true,
    );
    try {
      server.sendCustomPacket(
        userId,
        JSON.stringify({
          customPacketType: 'nicknameVisibility',
          enabled: showCharacterNicknames,
        }),
      );
    } catch (e) {
      console.error(e);
    }
    for (const system of systems) {
      try {
        if (system.connect) {
          system.connect(userId, ctx);
        }
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on('disconnect', (userId: number) => {
    log('disconnect', userId);
    connectedUsers.delete(userId);

    try {
      const manager = (globalThis as any).voiceActivityManager;
      const actorId = String(server.getUserActor(userId) || '');
      if (manager && actorId) {
        manager.removePlayer(actorId);
      }
    } catch (e) {
      console.error('[Voice Activity] cleanup failed on disconnect:', e);
    }

    for (const system of systems) {
      try {
        if (system.disconnect) {
          system.disconnect(userId, ctx);
        }
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on('customPacket', (userId: number, rawContent: string) => {
    const content = JSON.parse(rawContent);

    const type = `${content.customPacketType}`;
    delete content.customPacketType;

    if (type === 'voiceActivity' || type === 'yaca:voiceActivity') {
      try {
        const senderActorId = Number(server.getUserActor(userId)) || userId;
        const parsedActorId = Number(content.actorId ?? content.playerId);
        const normalizedActorId = Number.isFinite(parsedActorId) && parsedActorId > 0
          ? parsedActorId
          : senderActorId;

        const state = {
          actorId: String(normalizedActorId),
          isSpeaking: Boolean(content.isSpeaking),
          voiceRange: Number(content.voiceRange ?? 8),
          providerId:
            typeof content.providerId === 'string'
              ? content.providerId
              : 'yaca-ts',
          timestamp: Number.isFinite(Number(content.timestamp))
            ? Number(content.timestamp)
            : Date.now(),
          metadata:
            content.metadata && typeof content.metadata === 'object'
              ? (content.metadata as Record<string, unknown>)
              : undefined,
        } as VoiceActivityState;

        const manager = (globalThis as any).voiceActivityManager;
        if (manager) {
          manager.broadcastVoiceActivity(state).catch((e: unknown) => {
            console.error('[Voice Activity] broadcast failed:', e);
          });
        }
      } catch (e) {
        console.error('[Voice Activity] invalid voiceActivity packet:', e);
      }
      return;
    }

    if (type === 'clientTelemetry') {
      const source =
        typeof content.source === 'string'
          ? String(content.source).slice(0, 80)
          : 'skymp5-client';
      const sessionId =
        typeof content.sessionId === 'string'
          ? String(content.sessionId).slice(0, 64)
          : undefined;
      const receivedAt = Date.now();

      const events = Array.isArray(content.events) ? content.events : [content];
      const ip = (() => {
        try {
          return ctx.svr.getUserIp(userId);
        } catch {
          return '';
        }
      })();

      const safeEvents = events.slice(0, 50).map((entry: any) => {
        const eventName = String(entry?.name ?? 'unknown').slice(0, 120);
        const levelRaw = String(entry?.level ?? 'info').toLowerCase();
        const level: 'info' | 'warn' | 'error' =
          levelRaw === 'warn' || levelRaw === 'error' || levelRaw === 'info'
            ? levelRaw
            : 'info';

        let details = '';
        if (typeof entry?.details === 'string') {
          details = entry.details;
        } else if (entry?.details !== undefined) {
          try {
            details = JSON.stringify(entry.details);
          } catch {
            details = String(entry.details);
          }
        }

        const tsRaw = Number(entry?.ts);

        return {
          userId,
          ip,
          source,
          sessionId,
          event: eventName,
          level,
          ts: Number.isFinite(tsRaw) ? tsRaw : receivedAt,
          receivedAt,
          details,
        };
      });

      ui.pushClientRuntimeEvents(safeEvents);
    }

    for (const system of systems) {
      try {
        if (system.customPacket)
          system.customPacket(userId, type, content, ctx);
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on('customPacket', (userId: number, content: string) => {
    // At this moment we don't have any custom packets
  });

  // It's important to call this before gamemode
  try {
    server.attachSaveStorage();
  } catch (e) {
    console.error(e);
    console.error(`Stopping the server due to the previous error`);
    process.exit(-1);
  }

  const allSettings = settingsObject.allSettings as Record<string, unknown>;
  const pluginDiscoveryObject =
    allSettings && typeof allSettings.pluginDiscovery === 'object'
      ? (allSettings.pluginDiscovery as Record<string, unknown>)
      : null;
  const pluginDiscoveryModeRaw = pluginDiscoveryObject?.mode;
  const pluginDiscoveryMode: 'prompt' | 'safe' =
    pluginDiscoveryModeRaw === 'prompt' ? 'prompt' : 'safe';

  const pluginsLoadOrderRaw = allSettings?.pluginsLoadOrder;
  const pluginsLoadOrder = Array.isArray(pluginsLoadOrderRaw)
    ? pluginsLoadOrderRaw.filter((v): v is string => typeof v === 'string')
    : [];

  const abortOnPluginError = pluginDiscoveryObject?.abortOnPluginError === true;
  const processCommandAllowlistRaw =
    pluginDiscoveryObject?.processCommandAllowlist;
  const processCommandAllowlist = Array.isArray(processCommandAllowlistRaw)
    ? processCommandAllowlistRaw
        .filter((v): v is string => typeof v === 'string')
        .map((v) => v.trim())
        .filter((v) => v.length > 0)
    : null;

  const voiceActivityObject =
    allSettings && typeof allSettings.voiceActivity === 'object'
      ? (allSettings.voiceActivity as Record<string, unknown>)
      : null;
  const voiceActivityEnabled = voiceActivityObject?.enabled === true;

  if (voiceActivityEnabled) {
    const voiceManager = initializeVoiceActivityManager({
      enabled: true,
      voiceRangeTiers: Array.isArray(voiceActivityObject?.voiceRangeTiers)
        ? (voiceActivityObject.voiceRangeTiers as unknown[])
            .filter((v): v is number => typeof v === 'number')
        : undefined,
      defaultVoiceRange:
        typeof voiceActivityObject?.defaultVoiceRange === 'number'
          ? (voiceActivityObject.defaultVoiceRange as number)
          : undefined,
      proximityDistance:
        typeof voiceActivityObject?.proximityDistance === 'number'
          ? (voiceActivityObject.proximityDistance as number)
          : undefined,
      inactivityTimeoutMs:
        typeof voiceActivityObject?.inactivityTimeoutMs === 'number'
          ? (voiceActivityObject.inactivityTimeoutMs as number)
          : undefined,
      allowedProviders: Array.isArray(voiceActivityObject?.allowedProviders)
        ? (voiceActivityObject.allowedProviders as unknown[])
            .filter((v): v is string => typeof v === 'string')
        : null,
    });

    (globalThis as any).voiceActivityManager = voiceManager;
    voiceManager.setBroadcastCallback(async (state, recipientUserIds) => {
      const packet = JSON.stringify({
        customPacketType: 'syncVoiceActivity',
        actorId: state.actorId,
        isSpeaking: state.isSpeaking,
        voiceRange: state.voiceRange,
        providerId: state.providerId,
        timestamp: state.timestamp,
      });

      for (const userId of recipientUserIds) {
        try {
          server.sendCustomPacket(Number(userId), packet);
        } catch (e) {
          console.error('[Voice Activity] sendCustomPacket failed:', e);
        }
      }
    }, (state) => {
      const speakerActorId = Number(state.actorId);
      if (!Number.isFinite(speakerActorId) || speakerActorId <= 0) {
        return Array.from(connectedUsers).map((id) => String(id));
      }

      let speakerPos: number[] = [];
      let speakerCell = 0;
      try {
        speakerPos = server.getActorPos(speakerActorId) || [];
        speakerCell = Number(server.getActorCellOrWorld(speakerActorId)) || 0;
      } catch (e) {
        console.error('[Voice Activity] Failed to resolve speaker position:', e);
        return Array.from(connectedUsers).map((id) => String(id));
      }

      const maxDistance = Math.max(
        0,
        Math.min(
          state.voiceRange || 0,
          typeof voiceActivityObject?.proximityDistance === 'number'
            ? (voiceActivityObject.proximityDistance as number)
            : 100,
        ),
      );

      if (speakerPos.length < 3) {
        return Array.from(connectedUsers).map((id) => String(id));
      }

      const recipients: string[] = [];
      for (const userId of connectedUsers) {
        try {
          const targetActorId = Number(server.getUserActor(userId)) || 0;
          if (!targetActorId || targetActorId === speakerActorId) {
            continue;
          }

          const targetCell = Number(server.getActorCellOrWorld(targetActorId)) || 0;
          if (targetCell !== speakerCell) {
            continue;
          }

          const targetPos = server.getActorPos(targetActorId) || [];
          if (targetPos.length < 3) {
            continue;
          }

          const dx = targetPos[0] - speakerPos[0];
          const dy = targetPos[1] - speakerPos[1];
          const dz = targetPos[2] - speakerPos[2];
          const distance = Math.sqrt(dx * dx + dy * dy + dz * dz);

          if (distance <= maxDistance) {
            recipients.push(String(userId));
          }
        } catch (e) {
          console.error('[Voice Activity] Failed to resolve recipient:', e);
        }
      }

      return recipients;
    });

    const providerList = Array.isArray(voiceActivityObject?.providers)
      ? (voiceActivityObject.providers as unknown[])
          .filter((v): v is string => typeof v === 'string')
      : ['yaca-ts'];

    if (providerList.includes('yaca-ts')) {
      const yacaAdapter = createYacaTeamSpeakAdapter();
      void voiceManager.registerAdapter(yacaAdapter).catch((e) => {
        console.error('[Voice Activity] Failed to register YACA adapter:', e);
      });

      (globalThis as any).yacaTeamSpeakAdapter = yacaAdapter;
    }
  }

  await setupGamemode(server, gamemodePath, {
    mode: pluginDiscoveryMode,
    loadOrder: pluginsLoadOrder,
    abortOnPluginError,
    processCommandAllowlist,
  });
};

main();

// This is needed at least to handle axios errors in masterClient
// TODO: implement alerts
process.on('unhandledRejection', (...args) => {
  console.error('[!!!] unhandledRejection');
  console.error(...args);
});

// setTimeout on gamemode should not be able to kill the entire server
// TODO: implement alerts
process.on('uncaughtException', (...args) => {
  console.error('[!!!] uncaughtException');
  console.error(...args);
});
