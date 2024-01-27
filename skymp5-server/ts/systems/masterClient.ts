import { System, Log } from "./system";
import Axios from "axios";
import { SystemContext } from "./system";
import { getMyPublicIp } from "../publicIp";
import { ScampServer } from "../scampNative";

export class MasterClient implements System {
  systemName = "MasterClient";

  constructor(
    private log: Log,
    private serverPort: number,
    private masterUrl: string | null,
    private maxPlayers: number,
    private name: string,
    private ip: string,
    private updateIntervalMs = 5000,
    private offlineMode = false
  ) { }

  async initAsync(): Promise<void> {
    if (!this.masterUrl) return this.log("No master server specified");

    this.log(`Using master server on ${this.masterUrl}`);

    let myAddr: string;
    if (this.ip && this.ip != "null") myAddr = this.ip + ":" + this.serverPort;
    else myAddr = (await getMyPublicIp()) + ":" + this.serverPort;

    this.endpoint = `${this.masterUrl}/api/servers/${myAddr}`;
    this.log(`Our endpoint on master is ${this.endpoint}`);
  }

  update(): void {
    return;
  }

  async updateAsync(ctx: SystemContext): Promise<void> {
    if (this.offlineMode) return;

    await new Promise((r) => setTimeout(r, this.updateIntervalMs));

    if (this.endpoint) {
      const { name, maxPlayers } = this;
      const online = this.getCurrentOnline(ctx.svr);
      try {
        await Axios.post(this.endpoint, { name, maxPlayers, online });
      } catch (e) {
        console.error(`Error updating info on master server: ${e}`);
      }
    }
  }

  // connect/disconnect events are not reliable so we do full recalculate
  private getCurrentOnline(svr: ScampServer): number {
    return (svr as any).get(0, "onlinePlayers").length;
  }

  customPacket(): void {
    return;
  }

  private endpoint: string;
}
