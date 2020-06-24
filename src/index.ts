import * as sourceMapSupport from "source-map-support";
sourceMapSupport.install();

import * as scampNative from "./scampNative";

const server = new scampNative.ScampServer(7777, 1000);

server.on("connect", (userId: number) => {
  console.log("connect", userId);

  const formId = 0xff000000 + userId;
  server.createActor(formId, [163113.0938, -62752.3008, 7487.8579], 0, 0x3c);
  server.setUserActor(userId, formId);
  console.log("hey");
});

server.on("disconnect", (userId: number) => {
  console.log("disconnect", userId);
  //server.destroyActor(server.getUserActor(userId));
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
