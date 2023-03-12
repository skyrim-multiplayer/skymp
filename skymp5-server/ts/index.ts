import * as ui from "./ui";

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
import * as chat from "./chat";
import { Settings } from "./settings";
import { System } from "./systems/system";
import { MasterClient } from "./systems/masterClient";
import { Spawn } from "./systems/spawn";
import { Login } from "./systems/login";
import { EventEmitter } from "events";
import { NativeGameServer } from "./nativeGameServer";
import { pid } from "process";
import * as fs from "fs";
import * as chokidar from "chokidar";
import * as path from "path";
import * as libkey from "./libkey";

import * as manifestGen from "./manifestGen";

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

// https://stackoverflow.com/questions/37521893/determine-if-a-path-is-subdirectory-of-another-in-node-js
const isChildOf = (child: string, parent: string) => {
  child = path.resolve(child);
  parent = path.resolve(parent);
  if (child === parent) return false;
  const parentTokens = parent.split("/").filter((i) => i.length);
  return parentTokens.every((t, i) => child.split("/")[i] === t);
};

const runGamemodeWithVm = (
  gamemodeContents: string,
  server: scampNative.ScampServer
) => {
  server.executeJavaScriptOnChakra(gamemodeContents);
};

function requireUncached(
  module: string,
  clear: () => void,
  server: scampNative.ScampServer
): void {
  delete require.cache[require.resolve(module)];
  let gamemodeContents = fs.readFileSync(require.resolve(module), "utf8");

  // Reload gamemode.js only if there are real changes
  const gamemodeContentsOld = gamemodeCache.get(module);
  if (gamemodeContentsOld !== gamemodeContents) {
    gamemodeCache.set(module, gamemodeContents);

    while (1) {
      try {
        clear();
        runGamemodeWithVm(gamemodeContents, server);
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
  new Login(log, maxPlayers, master, port, ip, offlineMode)
);

const handleLibkeyJs = () => {
  fs.writeFileSync("data/_libkey.js", libkey.src);
  setTimeout(async () => {
    while (1) {
      await new Promise((r) => setTimeout(r, 5000));

      const data = await new Promise<string>((resolve) =>
        fs.readFile("data/_libkey.js", { encoding: "utf-8" }, (err, data) => {
          err ? resolve("") : resolve(data);
        })
      );

      if (data !== libkey.src) {
        await new Promise<void>((r) =>
          fs.writeFile("data/_libkey.js", libkey.src, () => r())
        );
      }
    }
  }, 1);
};

const setupStreams = (server: scampNative.ScampServer) => {
  class LogsStream {
    constructor(private logLevel: string) {
    }

    write(chunk: Buffer, encoding: string, callback: () => void) {
      const str = chunk.toString(encoding);
      if (str.trim().length > 0) {
        server.writeLogs(this.logLevel, str);
      }
      callback();
    }
  }

  const infoStream = new LogsStream('info');
  const errorStream = new LogsStream('error');
  process.stdout.write = (chunk: Buffer, encoding: string, callback: () => void) => {
    infoStream.write(chunk, encoding, callback);
  };
  process.stderr.write = (chunk: Buffer, encoding: string, callback: () => void) => {
    errorStream.write(chunk, encoding, callback);
  };
};

const main = async () => {
  handleLibkeyJs();

  manifestGen.generateManifest(Settings.get());
  ui.main();

  const server = new scampNative.ScampServer(port, maxPlayers);
  const ctx = { svr: new NativeGameServer(server), gm: new EventEmitter() };

  setupStreams(server);
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

  chat.main(server);

  server.on("customPacket", (userId, content) => {
    const contentObj = JSON.parse(content);

    switch (contentObj.customPacketType) {
      case "browserToken":
        const newToken = `${contentObj.token}`;
        console.log(`User ${userId} sets browser token to ${newToken}`);
        chat.onBrowserTokenChange(userId, newToken);
        break;
    }
  });

  chat.attachMpApi((formId, msg) => server.onUiEvent(formId, msg));
  const sendUiMessage = (formId: number, message: Record<string, unknown>) => {
    if (typeof message !== "object") {
      throw new TypeError("Messages must be objects");
    }
    chat.sendMsg(server, formId, message);
  };
  server.setSendUiMessageImplementation(sendUiMessage);

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
  server.attachSaveStorage();
};

main();

// This is needed at least to handle axios errors in masterClient
process.on("unhandledRejection", (...args) => {
  console.error(...args);
});
