import { ArgumentParser } from 'argparse';
import * as fs from 'fs';

export interface DiscordAuthSettings {
  botToken: string;
  guildId: string;
  banRoleId: string;
  eventLogChannelId?: string;
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
      pos: [22659, -8697, -3594],
      worldOrCell: '0x1a26f',
      angleZ: 268,
    },
  ];
  discordAuth: DiscordAuthSettings | null = null;

  constructor() {
    if (fs.existsSync('./skymp5-gamemode')) {
      this.gamemodePath = './skymp5-gamemode/gamemode.js';
    } else {
      this.gamemodePath = './gamemode.js';
    }

    if (fs.existsSync('./server-settings.json')) {
      const parsed = JSON.parse(fs.readFileSync('./server-settings.json', 'utf-8'));
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
      ].forEach((prop) => {
        if (parsed[prop]) (this as Record<string, unknown>)[prop] = parsed[prop];
      });
    }
  }

  static cachedSettings: Settings | null = null;

  static get(): Settings {
    if (Settings.cachedSettings) {
      return Settings.cachedSettings;
    }
    const args = Settings.parseArgs();
    const res = new Settings();

    res.port = +args['port'] || res.port;
    res.maxPlayers = +args['maxPlayers'] || res.maxPlayers;
    res.master = args['master'] || res.master;
    res.name = args['name'] || res.name;
    res.ip = args['ip'] || res.ip;
    res.offlineMode = args['offlineMode'] || res.offlineMode;
    Settings.cachedSettings = res;
    return res;
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
    return parser.parse_args();
  }
}
