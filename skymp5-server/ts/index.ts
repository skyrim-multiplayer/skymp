import * as ui from "./ui";

// @ts-ignore
import * as sourceMapSupport from "source-map-support";
sourceMapSupport.install({
  retrieveSourceMap: function (source: string) {
    if (source.endsWith('skymp5-server.js')) {
      return {
        url: 'original.js',
        map: require('fs').readFileSync('dist_back/skymp5-server.js.map', 'utf8')
      };
    }
    return null;
  }
});

import * as scampNative from "./scampNative";
import { Settings } from "./settings";
import { System } from "./systems/system";
import { MasterClient } from "./systems/masterClient";
import { Spawn } from "./systems/spawn";
import { Login } from "./systems/login";
import { EventEmitter } from "events";
import { pid } from "process";
import * as fs from "fs";
import * as chokidar from "chokidar";
import * as path from "path";
import * as os from "os";
import * as crypto from "crypto";

import * as manifestGen from "./manifestGen";
import { DiscordBanSystem } from "./systems/discordBanSystem";
import { createScampServer } from "./scampNative";
import { Octokit } from "@octokit/rest";
import * as lodash from "lodash";

const {
  master,
  port,
  maxPlayers,
  name,
  ip,
  gamemodePath,
  offlineMode,
} = Settings.get();

const gamemodeCache = new Map<string, string>();

function requireTemp(module: string) {
  // https://blog.mastykarz.nl/create-temp-directory-app-node-js/
  let tmpDir;
  const appPrefix = 'skymp5-server';
  try {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), appPrefix));

    const contents = fs.readFileSync(module, 'utf8');
    const tempPath = path.join(tmpDir, Math.random() + '-' + Date.now() + '.js');
    fs.writeFileSync(tempPath, contents);

    require(tempPath);
  }
  catch (e) {
    console.error(e.stack);
  }
  finally {
    try {
      if (tmpDir) {
        fs.rmSync(tmpDir, { recursive: true });
      }
    }
    catch (e) {
      console.error(`An error has occurred while removing the temp folder at ${tmpDir}. Please remove it manually. Error: ${e}`);
    }
  }
}

function requireUncached(
  module: string,
  clear: () => void,
  server: scampNative.ScampServer
): void {
  let gamemodeContents = fs.readFileSync(require.resolve(module), "utf8");

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
          console.log("Bad syntax, ignoring");
          return;
        }
      }
    }
  }
}

const log = console.log;
const systems = new Array<System>();
systems.push(
  new MasterClient(log, port, master, maxPlayers, name, ip, 5000, offlineMode),
  new Spawn(log),
  new Login(log, maxPlayers, master, port, ip, offlineMode),
  new DiscordBanSystem()
);

const setupStreams = (scampNative: any) => {
  class LogsStream {
    constructor(private logLevel: string) {
    }

    write(chunk: Buffer, encoding: string, callback: () => void) {
      // @ts-ignore
      const str = chunk.toString(encoding);
      if (str.trim().length > 0) {
        scampNative.writeLogs(this.logLevel, str);
      }
      callback();
    }
  }

  const infoStream = new LogsStream('info');
  const errorStream = new LogsStream('error');
  // @ts-ignore
  process.stdout.write = (chunk: Buffer, encoding: string, callback: () => void) => {
    infoStream.write(chunk, encoding, callback);
  };
  // @ts-ignore
  process.stderr.write = (chunk: Buffer, encoding: string, callback: () => void) => {
    errorStream.write(chunk, encoding, callback);
  };
};

/**
 * Resolves a Git ref to a commit hash if it's not already a commit hash.
 */
async function resolveRefToCommitHash(octokit: Octokit, owner: string, repo: string, ref: string): Promise<string> {
  // Check if `ref` is already a 40-character hexadecimal string (commit hash).
  if (/^[a-f0-9]{40}$/i.test(ref)) {
    return ref; // It's already a commit hash.
  }

  // Attempt to resolve the ref as both a branch and a tag.
  try {
    // First, try to resolve it as a branch.
    return await getCommitHashFromRef(octokit, owner, repo, `heads/${ref}`);
  } catch (error) {
    try {
      // If the branch resolution fails, try to resolve it as a tag.
      return await getCommitHashFromRef(octokit, owner, repo, `tags/${ref}`);
    } catch (tagError) {
      throw new Error('Could not resolve ref to commit hash.');
    }
  }
}

async function getCommitHashFromRef(octokit: Octokit, owner: string, repo: string, ref: string): Promise<string> {
  const { data } = await octokit.git.getRef({
    owner,
    repo,
    ref,
  });
  return data.object.sha;
}

async function fetchServerSettings(): Promise<any> {
  // Load server-settings.json
  const settingsPath = 'server-settings.json';
  const rawSettings = fs.readFileSync(settingsPath, 'utf8');
  let serverSettingsFile = JSON.parse(rawSettings);

  let serverSettings: Record<string, unknown> = {};

  const additionalServerSettings = serverSettingsFile.additionalServerSettings || [];

  let dumpFileNameSuffix = '';

  for (let i = 0; i < additionalServerSettings.length; ++i) {
    console.log(`Verifying additional server settings source ${i + 1} / ${additionalServerSettings.length}`);

    const { type, repo, ref, token, pathRegex } = serverSettingsFile.additionalServerSettings[i];

    if (typeof type !== "string") {
      throw new Error(`Expected additionalServerSettings[${i}].type to be string`);
    }

    if (type !== "github") {
      throw new Error(`Expected additionalServerSettings[${i}].type to be one of ["github"], but got ${type}`);
    }

    if (typeof repo !== "string") {
      throw new Error(`Expected additionalServerSettings[${i}].repo to be string`);
    }
    if (typeof ref !== "string") {
      throw new Error(`Expected additionalServerSettings[${i}].ref to be string`);
    }
    if (typeof token !== "string") {
      throw new Error(`Expected additionalServerSettings[${i}].token to be string`);
    }
    if (typeof pathRegex !== "string") {
      throw new Error(`Expected additionalServerSettings[${i}].pathRegex to be string`);
    }

    const octokit = new Octokit({ auth: token });

    const [owner, repoName] = repo.split('/');

    const commitHash = await resolveRefToCommitHash(octokit, owner, repoName, ref);
    dumpFileNameSuffix += `-${commitHash}`;
  }

  const dumpFileName = `server-settings-dump.json`;

  const readDump: Record<string, unknown> | undefined = fs.existsSync(dumpFileName) ? JSON.parse(fs.readFileSync(dumpFileName, 'utf-8')) : undefined;

  let readDumpNoSha512 = structuredClone(readDump);
  if (readDumpNoSha512) {
    delete readDumpNoSha512['_sha512_'];
  }

  const expectedSha512 = readDumpNoSha512 ? crypto.createHash('sha512').update(JSON.stringify(readDumpNoSha512)).digest('hex') : '';

  if (readDump && readDump["_meta_"] === dumpFileNameSuffix && readDump["_sha512_"] === expectedSha512) {
    console.log(`Loading settings dump from ${dumpFileName}`);
    serverSettings = JSON.parse(fs.readFileSync(dumpFileName, 'utf-8'));
  }
  else {
    for (let i = 0; i < additionalServerSettings.length; ++i) {

      const { repo, ref, token, pathRegex } = serverSettingsFile.additionalServerSettings[i];

      console.log(`Fetching settings from "${repo}" at ref "${ref}" with path regex ${pathRegex}`);

      const regex = new RegExp(pathRegex);

      const octokit = new Octokit({ auth: token });

      const [owner, repoName] = repo.split('/');

      // List repository contents at specified ref
      const rootContent = await octokit.repos.getContent({
        owner,
        repo: repoName,
        ref,
        path: '',
      });

      const { data } = rootContent;

      const rateLimitRemainingInitial = parseInt(rootContent.headers["x-ratelimit-remaining"]) + 1;
      let rateLimitRemaining = 0;

      const onFile = async (file: { path: string, name: string }) => {
        if (file.name.endsWith('.json')) {
          if (regex.test(file.path)) {
            // Fetch individual file content if it matches the regex
            const fileData = await octokit.repos.getContent({
              owner,
              repo: repoName,
              ref,
              path: file.path,
            });
            rateLimitRemaining = parseInt(fileData.headers["x-ratelimit-remaining"]);

            if ('content' in fileData.data && typeof fileData.data.content === 'string') {
              // Decode Base64 content and parse JSON
              const content = Buffer.from(fileData.data.content, 'base64').toString('utf-8');
              const jsonContent = JSON.parse(content);
              // Merge or handle the JSON content as needed
              console.log(`Merging "${file.path}"`);

              serverSettings = lodash.merge(serverSettings, jsonContent);
            }
            else {
              throw new Error(`Expected content to be an array (${file.path})`);
            }
          }
          else {
            console.log(`Ignoring "${file.path}"`);
          }
        }
      }

      const onDir = async (file: { path: string, name: string }) => {
        const fileData = await octokit.repos.getContent({
          owner,
          repo: repoName,
          ref,
          path: file.path,
        });
        rateLimitRemaining = parseInt(fileData.headers["x-ratelimit-remaining"]);

        if (Array.isArray(fileData.data)) {
          for (const item of fileData.data) {
            if (item.type === "file") {
              await onFile(item);
            }
            else if (item.type === "dir") {
              await onDir(item);
            }
            else {
              console.warn(`Skipping unsupported item type ${item.type} (${item.path})`);
            }
          }
        }
        else {
          throw new Error(`Expected data to be an array (${file.path})`);
        }
      }

      if (Array.isArray(data)) {
        for (const item of data) {
          if (item.type === "file") {
            await onFile(item);
          }
          else if (item.type === "dir") {
            await onDir(item);
          }
          else {
            console.warn(`Skipping unsupported item type ${item.type} (${item.path})`);
          }
        }
      }
      else {
        throw new Error(`Expected data to be an array (root)`);
      }

      console.log(`Rate limit spent: ${rateLimitRemainingInitial - rateLimitRemaining}, remaining: ${rateLimitRemaining}`);

      const xRateLimitReset = rootContent.headers["x-ratelimit-reset"];
      const resetDate = new Date(parseInt(xRateLimitReset, 10) * 1000);
      const currentDate = new Date();
      if (resetDate > currentDate) {
        console.log("The rate limit will reset in the future");
        const secondsUntilReset = (resetDate.getTime() - currentDate.getTime()) / 1000;
        console.log(`Seconds until reset: ${secondsUntilReset}`);
      } else {
        console.log("The rate limit has already been reset");
      }
    }

    if (JSON.stringify(serverSettings) !== JSON.stringify(JSON.parse(rawSettings))) {
      console.log(`Dumping ${dumpFileName} for cache and debugging`);
      serverSettings["_meta_"] = dumpFileNameSuffix;
      serverSettings["_sha512_"] = crypto.createHash('sha512').update(JSON.stringify(serverSettings)).digest('hex');
      fs.writeFileSync(dumpFileName, JSON.stringify(serverSettings, null, 2));
    }
  }

  console.log(`Merging "server-settings.json" (original settings file)`);
  serverSettings = lodash.merge(serverSettings, serverSettingsFile);

  return serverSettings;
}

const main = async () => {
  setupStreams(scampNative.getScampNative());

  manifestGen.generateManifest(Settings.get());
  ui.main();

  let server: any;

  try {
    const serverSettings = await fetchServerSettings();
    server = createScampServer(port, maxPlayers, serverSettings);
  }
  catch (e) {
    console.error(e);
    console.error(`Stopping the server due to the previous error`);
    process.exit(-1);
  }
  const ctx = { svr: server, gm: new EventEmitter() };

  console.log(`Current process ID is ${pid}`);

  (async () => {
    while (1) {
      try {
        server.tick();
        await new Promise((r) => setTimeout(r, 1));
      } catch (e) {
        console.error(`in server.tick:\n${e.stack}`);
      }
    }
  })();

  for (const system of systems) {
    if (system.initAsync) await system.initAsync(ctx);
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

  server.on("connect", (userId: number) => {
    log("connect", userId);
    for (const system of systems) {
      try {
        if (system.connect) system.connect(userId, ctx);
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on("disconnect", (userId: number) => {
    log("disconnect", userId);
    for (const system of systems) {
      try {
        if (system.disconnect) system.disconnect(userId, ctx);
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on("customPacket", (userId: number, rawContent: string) => {
    const content = JSON.parse(rawContent);

    const type = `${content.customPacketType}`;
    delete content.customPacketType;

    for (const system of systems) {
      try {
        if (system.customPacket)
          system.customPacket(userId, type, content, ctx);
      } catch (e) {
        console.error(e);
      }
    }
  });

  server.on("customPacket", (userId: number, content: string) => {
    // At this moment we don't have any custom packets
  });

  // It's important to call this before gamemode
  try {
    server.attachSaveStorage();
  }
  catch (e) {
    console.error(e);
    console.error(`Stopping the server due to the previous error`);
    process.exit(-1);
  }

  const clear = () => server.clear();

  const toAbsolute = (p: string) => {
    if (path.isAbsolute(p)) return p;
    return path.resolve("", p);
  };

  const absoluteGamemodePath = toAbsolute(gamemodePath);
  log(`Gamemode path is "${absoluteGamemodePath}"`);

  if (!fs.existsSync(absoluteGamemodePath)) {
    log(
      `Error during loading a gamemode from "${absoluteGamemodePath}" - file or directory does not exist`
    );
  } else {
    try {
      requireUncached(absoluteGamemodePath, clear, server);
    } catch (e) {
      console.error(e);
    }
  }

  if (fs.existsSync(absoluteGamemodePath)) {
    try {
      requireUncached(absoluteGamemodePath, clear, server);
    } catch (e) {
      console.error(e);
    }

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
      setTimeout(
        () => (n === numReloads.n ? reloadGamemode() : undefined),
        1000
      );
    };

    watcher.on("add", reloadGamemodeTimeout);
    watcher.on("addDir", reloadGamemodeTimeout);
    watcher.on("change", reloadGamemodeTimeout);
    watcher.on("unlink", reloadGamemodeTimeout);
    watcher.on("error", function (error) {
      console.error("Error happened in chokidar watch", error);
    });
  }
};

main();

// This is needed at least to handle axios errors in masterClient
process.on("unhandledRejection", (...args) => {
  console.error(...args);
});
