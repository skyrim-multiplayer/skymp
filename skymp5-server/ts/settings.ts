import * as fs from 'fs';
import * as crypto from "crypto";
import { Octokit } from '@octokit/rest';
import { ArgumentParser } from 'argparse';
import lodash from 'lodash';

export interface DiscordAuthSettings {
  botToken: string;
  guildId: string;
  banRoleId: string;
  eventLogChannelId?: string;
  hideIpRoleId?: string;
}

export class Settings {
  ip: string | null = null;
  port = 7777;
  maxPlayers = 100;
  master: string = "https://sweetpie.nic11.xyz";
  name = 'Yet Another Server';
  gamemodePath = '...';
  loadOrder = new Array<string>();
  dataDir = './data';
  offlineMode = false;
  startPoints = [
    {
      pos: [133857, -61130, 14662],
      worldOrCell: '0x3c',
      angleZ: 72,
    },
  ];
  databaseDriver: string | undefined = undefined;
  databaseName: string | undefined = undefined;
  databaseUri: string | undefined = undefined;
  databaseRedisCacheUri: string | undefined = undefined;
  logLevel: string | undefined = undefined;

  discordAuth: DiscordAuthSettings | null = null;

  allSettings: Record<string, unknown> | null = null;

  constructor() { }

  static async get(): Promise<Settings> {
    if (!Settings.cachedPromise) {
      Settings.cachedPromise = (async () => {
        const args = Settings.parseArgs();
        const res = new Settings();

        await res.loadSettings();  // Load settings asynchronously

        // Override settings with command line arguments if available
        res.allSettings.port = res.port = +args['port'] || res.port;
        res.allSettings.maxPlayers = res.maxPlayers = +args['maxPlayers'] || res.maxPlayers;
        res.allSettings.master = res.master = args['master'] || res.master;
        res.allSettings.name = res.name = args['name'] || res.name;
        res.allSettings.ip = res.ip = args['ip'] || res.ip;
        res.allSettings.offlineMode = res.offlineMode = args['offlineMode'] || res.offlineMode;
        res.allSettings.gamemodePath = res.gamemodePath = args['gamemodePath'] || res.gamemodePath;
        res.allSettings.databaseDriver = res.databaseDriver = args['databaseDriver'] || res.databaseDriver;
        res.allSettings.databaseName = res.databaseName = args['databaseName'] || res.databaseName;
        res.allSettings.databaseUri = res.databaseUri = args['databaseUri'] || res.databaseUri;
        res.allSettings.databaseRedisCacheUri = res.databaseRedisCacheUri = args['databaseRedisCacheUri'] || res.databaseRedisCacheUri;
        res.allSettings.logLevel = res.logLevel = args['logLevel'] || res.logLevel;

        return res;
      })();
    }

    return Settings.cachedPromise;
  }

  private async loadSettings() {
    if (fs.existsSync('./skymp5-gamemode')) {
      this.gamemodePath = './skymp5-gamemode/gamemode.js';
    } else {
      this.gamemodePath = './gamemode.js';
    }

    const settings = await fetchServerSettings();
    [
      'ip',
      'port',
      'maxPlayers',
      'master',
      'name',
      'gamemodePath',
      'loadOrder',
      'dataDir',
      'startPoints',
      'offlineMode',
      'discordAuth',
      'databaseDriver',
      'databaseName',
      'databaseUri',
      'databaseRedisCacheUri',
      'logLevel',
    ].forEach((prop) => {
      if (settings[prop]) (this as Record<string, unknown>)[prop] = settings[prop];
    });

    this.allSettings = settings;
  }

  private static parseArgs() {
    const parser = new ArgumentParser({
      add_help: false,
      description: '',
    });
    parser.add_argument('-m', '--maxPlayers');
    parser.add_argument('--master');
    parser.add_argument('--name');
    parser.add_argument('--port');
    parser.add_argument('--ip');
    parser.add_argument('--offlineMode');
    parser.add_argument('--gamemodePath');
    parser.add_argument('--databaseDriver');
    parser.add_argument('--databaseName');
    parser.add_argument('--databaseUri');
    parser.add_argument('--databaseRedisCacheUri');
    parser.add_argument('--logLevel');
    return parser.parse_args();
  }

  private static cachedPromise: Promise<Settings> | null = null;
}

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

  fs.writeFileSync('server-settings-merged.json', JSON.stringify(serverSettings, null, 2));

  return serverSettings;
}
