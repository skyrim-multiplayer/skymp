import { ArgumentParser } from "argparse";
import * as fs from "fs";

export class Settings {
  ip: string | null = null;
  port = 7777;
  maxPlayers = 100;
  master: string | null = null;
  name = "Yet Another Server";
  gamemodePath = "...";
  loadOrder = new Array<string>();
  dataDir = "./data";
  offlineMode = false;
  startPoints = [
    {
      pos: [22659, -8697, -3594],
      worldOrCell: '0x1a26f',
      angleZ: 268
    },
  ];

  constructor() {
    if (fs.existsSync("./skymp5-gamemode")) {
      this.gamemodePath = "./skymp5-gamemode/gamemode.js";
    }
    else {
      this.gamemodePath = "./gamemode.js";
    }

    if (fs.existsSync("./server-settings.json")) {
      const parsed = JSON.parse(
        fs.readFileSync("./server-settings.json", "utf-8")
      );
      [
        "ip",
        "port",
        "maxPlayers",
        "master",
        "name",
        "gamemodePath",
        "loadOrder",
        "dataDir",
        "startPoints",
        "offlineMode",
      ].forEach((prop) => {
        if (parsed[prop])
          (this as Record<string, unknown>)[prop] = parsed[prop];
      });
    }
  }

  static get(): Settings {
    const args = Settings.parseArgs();
    const res = new Settings();

    res.port = +args["port"] || res.port;
    res.maxPlayers = +args["maxPlayers"] || res.maxPlayers;
    res.master = args["master"];
    res.name = args["name"] || res.name;
    res.ip = args["ip"] || res.ip;
    res.offlineMode = args["offlineMode"] || res.offlineMode;
    return res;
  }

  private static parseArgs() {
    const parser = new ArgumentParser({
      version: "0.0.0",
      addHelp: false,
      description: "",
    });
    parser.addArgument(["-m", "--maxPlayers"], {});
    parser.addArgument(["--master"], {});
    parser.addArgument(["--name"], {});
    parser.addArgument(["--port"], {});
    parser.addArgument(["--ip"], {});
    parser.addArgument(["--offlineMode"], {});
    return parser.parseArgs();
  }
}
