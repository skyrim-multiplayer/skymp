import * as ui from "./ui";
ui.main();
console.log("ui main called");

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

console.log(`Current process ID is ${pid}`);

const master = Settings.get().master || "https://skymp.io";

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

  // TODO: Transform Chat into system

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

  const clear = mp.clear as () => void;

  if (fs.existsSync("./gamemode.js")) {
    try {
      clear();
      eval(fs.readFileSync("./gamemode.js", "utf8"));
    } catch (e) {
      console.error(e);
    }
    fs.watch("./gamemode.js", {}, () => {
      try {
        clear();
        eval(fs.readFileSync("./gamemode.js", "utf8"));
      } catch (e) {
        console.error(e);
      }
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
