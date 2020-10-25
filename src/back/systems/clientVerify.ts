import { System, Log, Content, SystemContext } from "./system";
import * as fs from "fs";

export class ClientVerify implements System {
  systemName = "ClientVerify";

  constructor(
    private log: Log,
    private compiledFrontPath: string,
    private maxPlayers: number
  ) {}

  async initAsync(ctx: SystemContext): Promise<void> {
    this.hashVerified.length = this.maxPlayers;
    this.hashVerified.fill(false);

    fs.watch(this.compiledFrontPath, {}, () => {
      this.log("Front changed, reloading");
      this.reloadFront();
      this.connectedUserIds.forEach((userId) => {
        ctx.svr.sendCustomPacket(userId, "newClientVersion", {
          src: this.compiledFront,
        });
      });
    });

    this.reloadFront();
  }

  updateAsync(): Promise<void> {
    return;
  }

  connect(userId: number): void {
    this.connectedUserIds.add(userId);
  }

  disconnect(userId: number): void {
    this.hashVerified[userId] = false;
    this.connectedUserIds.delete(userId);
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext
  ): void {
    if (type !== "clientVersion") return;

    if (this.hashVerified[userId])
      return this.log(`User ${userId} is already verified, ignoring`);

    if (content["src"] === this.compiledFront) {
      this.hashVerified[userId] = true;
      this.log(`Verified front source code for ${userId}`);
      ctx.gm.emit("loginRequired", userId);
    } else {
      this.log(
        `Sending new front for ${userId} (${content["src"].length} !== ${this.compiledFront.length})`
      );
      ctx.svr.sendCustomPacket(userId, "newClientVersion", {
        src: this.compiledFront,
      });
    }
  }

  private reloadFront(): void {
    this.compiledFront = fs.readFileSync(this.compiledFrontPath, {
      encoding: "utf-8",
    });
  }

  private hashVerified = new Array<boolean>();
  private compiledFront: string;
  private connectedUserIds = new Set<number>();
}
