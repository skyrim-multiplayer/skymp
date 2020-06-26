const config =
  !process.env.NODE_ENV || process.env.NODE_ENV === "development"
    ? "Debug"
    : "Release";
console.log(`Using scamp_native config ${config}`);

export declare class ScampServer {
  constructor(serverPort: number, maxPlayers: number);

  on(event: "connect", handler: (userId: number) => void): void;
  on(event: "disconnect", handler: (userId: number) => void): void;
  on(
    event: "customPacket",
    handler: (userId: number, content: string) => void
  ): void;
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
}

module.exports.ScampServer = require(`../build/${config}/scamp_native.node`).ScampServer;
