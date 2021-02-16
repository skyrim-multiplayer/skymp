import * as ui from "./ui";

import * as sourceMapSupport from "source-map-support";
sourceMapSupport.install();

import * as scampNative from "./scampNative";
import * as chat from "./chat";
import { Settings } from "./settings";
import { System } from "./systems/system";
import { ClientVerify } from "./systems/clientVerify";
import { MasterClient } from "./systems/masterClient";
import { Spawn } from "./systems/spawn";
import { Login } from "./systems/login";
import { EventEmitter } from "events";
import { NativeGameServer } from "./nativeGameServer";
import { pid } from "process";
import * as fs from "fs";
import * as chokidar from "chokidar";
import * as path from "path";
import { ensureMastersAndScriptsPresent } from "./dataDownloader";

import * as manifestGen from "./manifestGen";

console.log(`Current process ID is ${pid}`);

const master = Settings.get().master || "https://skymp.io";

const gamemodeCache = new Map<string, string>();

function requireUncached(module: string): void {
  delete require.cache[require.resolve(module)];

  const gamemodeContents = fs.readFileSync(require.resolve(module), "utf8");

  // Reload gamemode.js only if there are real changes
  const gamemodeContentsOld = gamemodeCache.get(module);
  if (gamemodeContentsOld !== gamemodeContents) {
    gamemodeCache.set(module, gamemodeContents);
    require(module);
  }
}

const log = console.log;
const systems = new Array<System>();
systems.push(
  new MasterClient(
    log,
    Settings.get().port,
    master,
    Settings.get().maxPlayers,
    Settings.get().name,
    Settings.get().ip,
    5000
  ),
  new ClientVerify(
    log,
    "./dist_front/skymp5-client.js",
    Settings.get().maxPlayers
  ),
  new Spawn(log),
  new Login(
    log,
    Settings.get().maxPlayers,
    master,
    Settings.get().port,
    Settings.get().ip
  )
);

const main = async () => {
  await ensureMastersAndScriptsPresent(
    Settings.get().dataDir,
    Settings.get().loadOrder
  );
  manifestGen.generateManifest(Settings.get());
  ui.main();

  const server = new scampNative.ScampServer(
    Settings.get().port,
    Settings.get().maxPlayers
  );
  const ctx = { svr: new NativeGameServer(server), gm: new EventEmitter() };

  (async () => {
    while (1) {
      try {
        server.tick();
        await new Promise((r) => setTimeout(r, 1));
      } catch (e) {
        log(`Error in server.tick: ${e}\n${e.stack}`);
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
            log(`Error in ${system.systemName}.updateAsync: ${e}\n${e.stack}`);
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
        log(`Error in ${system.systemName}.connect: ${e}\n${e.stack}`);
      }
    }
  });

  server.on("disconnect", (userId: number) => {
    log("disconnect", userId);
    for (const system of systems) {
      try {
        if (system.disconnect) system.disconnect(userId, ctx);
      } catch (e) {
        log(`Error in ${system.systemName}.disconnect: ${e}\n${e.stack}`);
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
        log(`Error in ${system.systemName}.customPacket: ${e}\n${e.stack}`);
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

  const g = (global as unknown) as Record<string, unknown>;
  const mp = server.getMpApi();
  g.mp = mp;
  chat.attachMpApi(mp);
  mp.sendUiMessage = (formId: number, message: Record<string, unknown>) => {
    if (typeof message !== "object") {
      throw new TypeError("Messages must be objects");
    }
    chat.sendMsg(server, formId, message);
  };

  const clear = mp.clear as () => void;

  const toAbsolute = (p: string) => {
    if (path.isAbsolute(p)) return p;
    return path.resolve("", p);
  };

  const gamemodePath = toAbsolute(Settings.get().gamemodePath);
  log(`Gamemode path is'${gamemodePath}'`);

  if (!fs.existsSync(gamemodePath)) {
    log(
      `Error during loading a gamemode from '${gamemodePath}' - file or directory does not exist`
    );
  } else {
    try {
      clear();
      requireUncached(gamemodePath);
    } catch (e) {
      console.error(e);
    }
  }

  if (fs.existsSync(gamemodePath)) {
    try {
      clear();
      requireUncached(gamemodePath);
    } catch (e) {
      console.error(e);
    }

    const watcher = chokidar.watch(gamemodePath, {
      ignored: /^\./,
      persistent: true,
      awaitWriteFinish: true,
    });

    const numReloads = { n: 0 };

    const reloadGamemode = () => {
      try {
        clear();
        requireUncached(gamemodePath);
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

main().catch((e) => {
  log(`Main function threw an error: ${e}`);
  if (e["stack"]) log(e["stack"]);
  process.exit(-1);
});

process.on("unhandledRejection", (...args) => {
  console.error(...args);
});
