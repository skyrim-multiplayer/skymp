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
const maxPlayers = +args.maxPlayers || 30;
console.log(
  `The server is starting on port ${port} with maxPlayers=${maxPlayers}`
);
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

server.on("connect", (userId: number) => {
  totalOnline++;
  console.log("connect", userId);

  const formId = 0xff000000 + userId;
  server.createActor(formId, [163113.0938, -62752.3008, 7487.8579], 0, 0x3c);
  server.setUserActor(userId, formId);
  console.log("hey");
});

server.on("disconnect", (userId: number) => {
  totalOnline--;
  console.log("disconnect", userId);
  const actorId = server.getUserActor(userId);
  if (actorId !== 0) server.destroyActor(actorId);
});

server.on("customPacket", (userId: number, content: string) => {
  console.log("customPacket", userId, content);
});

(async () => {
  while (1) {
    await new Promise(setImmediate);
    server.tick();
  }
})();

process.on("unhandledRejection", console.error);
