let scampNativeNode;

import * as fs from "fs";

if (fs.existsSync(process.cwd() + "/scamp_native.node")) {
  console.log("Using scamp_native.node from server's dir");
  scampNativeNode = require(process.cwd() + "/scamp_native.node");
} else {
  const config =
    !process.env.NODE_ENV || process.env.NODE_ENV === "development"
      ? "Debug"
      : "Release";
  console.log(`Using scamp_native config ${config}`);
  scampNativeNode = require(`../build/${config}/scamp_native.node`);
}

export declare interface Bot {
  destroy(): void;
  send(msg: Record<string, unknown>): void;
}

export declare class ScampServer {
  constructor(serverPort: number, maxPlayers: number);

  on(event: "connect", handler: (userId: number) => void): void;
  on(event: "disconnect", handler: (userId: number) => void): void;
  on(
    event: "customPacket",
    handler: (userId: number, content: string) => void
  ): void;
  attachSaveStorage(): void;
  tick(): void;

  createActor(
    formId: number,
    pos: number[],
    angleZ: number,
    cellOrWorld: number
  ): void;

  destroyActor(formId: number): void;
  setUserActor(userId: number, actorFormId: number): void;
  getUserActor(userId: number): number;
  getActorName(actorId: number): string;
  getActorPos(actorId: number): number[];
  getActorCellOrWorld(actorId: number): number;
  setRaceMenuOpen(formId: number, open: boolean): void;
  sendCustomPacket(userId: number, jsonContent: string): void;
  setEnabled(actorId: number, enabled: boolean): void;
  getActorsByProfileId(profileId: number): number[];
  createBot(): Bot;
  getUserByActor(formId: number): number;

  getMpApi(): Record<string, unknown>;
}

module.exports.ScampServer = scampNativeNode.ScampServer;
