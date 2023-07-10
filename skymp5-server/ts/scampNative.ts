const scampNativeNode = require(process.cwd() + "/scam_native.node");

export declare interface Bot {
  destroy(): void;
  send(msg: Record<string, unknown>): void;
}

export type SendChatMessageFn = (
  formId: number,
  message: Record<string, unknown>
) => void;

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
    cellOrWorld: number,
    userProfileId?: number
  ): number;

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

  executeJavaScriptOnChakra(src: string): void;
  clear(): void;
  writeLogs(logLevel: string, message: string): void;
}

module.exports.ScampServer = scampNativeNode.ScampServer;
