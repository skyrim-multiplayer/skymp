import * as ui from "./ui";
ui.main();
console.log("ui main called");

import * as fs from "fs";

import * as sourceMapSupport from "source-map-support";
sourceMapSupport.install();

import { ArgumentParser } from "argparse";
const parser = new ArgumentParser({
  version: "0.0.0",
  addHelp: false,
  description: "",
});
parser.addArgument(["-m", "--maxPlayers"], {});
parser.addArgument(["--master"], {});
parser.addArgument(["--name"], {});
const args = parser.parseArgs();

import * as scampNative from "./scampNative";

const port = 7777;
const maxPlayers = +args.maxPlayers || 100;
const server = new scampNative.ScampServer(port, maxPlayers);

import Axios from "axios";

import { getMyPublicIp } from "./publicIp";

let totalOnline = 0;

const master = args.master;
if (!master) {
  console.log("No master server specified");
} else {
  console.log(`Using master server on ${master}`);
  (async () => {
    const myIp = await getMyPublicIp();
    const endpoint = `${master}/api/servers/${myIp}:${port}`;
    console.log(`Our endpoint on master is ${endpoint}`);
    while (1) {
      await new Promise((r) => setTimeout(r, 5000));
      try {
        await Axios.post(`${master}/api/servers/${myIp}:${port}`, {
          name: args.name,
          maxPlayers: maxPlayers,
          online: totalOnline,
        });
      } catch (e) {
        console.log(e.toString());
      }
    }
  })();
}

function randomInteger(min: number, max: number) {
  const rand = min + Math.random() * (max + 1 - min);
  return Math.floor(rand);
}

const onClientVerify = (userId: number) => {
  const formId = 0xff000000 + userId;

  const spawnpoints = [
    //[163113.0938, -62752.3008, 7487.8579],
    [171450.66, -62565.31, 7223.21],
  ];
  const idx = randomInteger(0, spawnpoints.length - 1);

  server.createActor(formId, spawnpoints[idx], 268, 0x3c);

  server.setUserActor(userId, formId);
  server.setRaceMenuOpen(formId, true);
};

const connectedUserIds = new Set<number>();
server.on("connect", (userId: number) => connectedUserIds.add(userId));
server.on("disconnect", (userId: number) => connectedUserIds.delete(userId));

server.on("connect", (userId: number) => {
  totalOnline++;
  console.log("connect", userId);
});

let hashVerified = new Array<boolean>();
hashVerified.length = maxPlayers;
hashVerified = hashVerified.map(() => false);

const compiledFrontPath = "./dist_front/skymp5-client.js";
let compiledFront = "";

const reloadFront = () => {
  compiledFront = fs.readFileSync(compiledFrontPath, { encoding: "utf-8" });
};

reloadFront();

const sendNewClient = (userId: number, src: string) => {
  server.sendCustomPacket(
    userId,
    JSON.stringify({
      customPacketType: "newClientVersion",
      src,
    })
  );
};

fs.watch(compiledFrontPath, {}, () => {
  console.log("Front changed, reloading");
  reloadFront();
  connectedUserIds.forEach((userId) => {
    sendNewClient(userId, compiledFront);
  });
});

server.on("disconnect", (userId) => {
  hashVerified[userId] = false;
});

server.on("customPacket", (userId, content) => {
  const contentObj = JSON.parse(content);

  switch (contentObj.customPacketType) {
    case "browserToken":
      const newToken = `${contentObj.token}`;
      console.log(`User ${userId} sets browser token to ${newToken}`);
      chat.onBrowserTokenChange(userId, newToken);
      break;
    case "clientVersion":
      if (hashVerified[userId]) {
        console.log(`User ${userId} is already verified, ignoring`);
        break;
      }
      if (typeof contentObj.src !== "string") {
        console.log(`User ${userId} sent invalid front source`);
        break;
      }
      const src = contentObj.src as string;
      if (src === compiledFront) {
        hashVerified[userId] = true;
        console.log(`Verified front source code for ${userId}`);
        onClientVerify(userId);
      } else {
        console.log(
          `Sending new front for ${userId} (${src.length} !== ${compiledFront.length})`
        );
        sendNewClient(userId, compiledFront);
      }
      break;
  }
});

server.on("disconnect", (userId: number) => {
  totalOnline--;
  console.log("disconnect", userId);
  const actorId = server.getUserActor(userId);
  if (actorId !== 0) server.destroyActor(actorId);
});

(async () => {
  while (1) {
    try {
      server.tick();
      await new Promise(setImmediate);
    } catch (e) {
      console.error("server.tick ", e);
    }
  }
})();

process.on("unhandledRejection", console.error);

import * as chat from "./chat";
chat.main(server);
