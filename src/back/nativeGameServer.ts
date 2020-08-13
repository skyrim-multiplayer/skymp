import { ServerInterface } from "./serverInterface";
import { ScampServer } from "./scampNative";

export class NativeGameServer implements ServerInterface {
  constructor(private svr: ScampServer) {}

  sendCustomPacket(
    userId: number,
    type: string,
    content: Record<string, unknown>
  ): void {
    content["customPacketType"] = type;
    this.svr.sendCustomPacket(userId, JSON.stringify(content));
    delete content["customPacketType"];
  }

  setRaceMenuOpen(...args: unknown[]): void {
    return this.svr.setRaceMenuOpen.call(this.svr, ...args);
  }
  createActor(...args: unknown[]): void {
    return this.svr.createActor.call(this.svr, ...args);
  }

  destroyActor(...args: unknown[]): void {
    return this.svr.destroyActor.call(this.svr, ...args);
  }

  setUserActor(...args: unknown[]): void {
    return this.svr.setUserActor.call(this.svr, ...args);
  }

  getUserActor(...args: unknown[]): number {
    return this.svr.getUserActor.call(this.svr, ...args);
  }
}
