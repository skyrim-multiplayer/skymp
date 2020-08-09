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
    //[171450.66, -62565.31, 7223.21],
    { pos: [22659, -8697, -3594], worldOrCell: 0x1a26f, angleZ: 268 },
  ];
  const idx = randomInteger(0, spawnpoints.length - 1);

  server.createActor(
    formId,
    spawnpoints[idx].pos,
    spawnpoints[idx].angleZ,
    spawnpoints[idx].worldOrCell
  );

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

// Increase this to start the server with bots
const numBots = 0;

const bots = new Array<scampNative.Bot>();
for (let i = 0; i < numBots; ++i) {
  bots.push(server.createBot());
}
setInterval(() => {
  bots.forEach((bot, i) => {
    bot.send({
      idx: i,
      t: 2,
      data: {
        pos: [
          171450 + (i % 10) * 128,
          -62565 + ((i / 10) % 10) * 128,
          7223 + 32,
        ],
        rot: [0, 0, 0],
        worldOrCell: 0x3c,
        isWeapDrawn: true,
      },
    });
  });
}, 500);

const numChanges = new Array<number>();
numChanges.length = numBots;
numChanges.fill(1);

const items: number[] = [
  0x0002acd2, // These three appears to be non-crashing
  0x000233e3, //
  0x00061cd6, //
  0x0200284d,
  0x0004dee3,
  0x0002ac6f,
];

function createInventory() {
  return {
    entries: items
      .filter(() => Math.random() > 0.5)
      .map(function (x) {
        return { baseId: x, count: 1, worn: true };
      }),
  };
}

setInterval(() => {
  bots.forEach((bot, i) => {
    bot.send({
      idx: i,
      t: 5,
      data: {
        numChanges: numChanges[i]++,
        inv: createInventory(),
      },
    });
  });
}, 50);

server.on("connect", (userId) => userId < numBots && onClientVerify(userId));
