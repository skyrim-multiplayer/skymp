import { ArgumentParser } from "argparse";

export class Settings {
  port = 7777;
  maxPlayers = 100;
  master: string | null = null;
  name = "Yet Another SkyMP Server";

  static get(): Settings {
    const args = Settings.parseArgs();
    const res = new Settings();

    res.port = +args["port"] || res.port;
    res.maxPlayers = +args["maxPlayers"] || res.maxPlayers;
    res.master = args["master"];
    res.name = "" + args["name"] || res.name;

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
    return parser.parseArgs();
  }
}
