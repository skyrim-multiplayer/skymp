import { System, Log } from "./system";
import Axios from "axios";

const getMyPublicIp = async (): Promise<string> => {
  const res = await Axios.request({
    url: "http://ipv4bot.whatismyipaddress.com",
  });
  return res.data;
};

export class MasterClient implements System {
  systemName = "MasterClient";

  constructor(
    private log: Log,
    private serverPort: number,
    private masterUrl: string | null,
    private maxPlayers: number,
    private name: string,
    private updateIntervalMs = 5000
  ) {}

  async initAsync(): Promise<void> {
    if (!this.masterUrl) return this.log("No master server specified");

    this.log(`Using master server on ${this.masterUrl}`);

    const serverIp = await getMyPublicIp();
    this.endpoint = `${this.masterUrl}/api/servers/${serverIp}:${this.serverPort}`;
    this.log(`Our endpoint on master is ${this.endpoint}`);
  }

  update(): void {
    return;
  }

  async updateAsync(): Promise<void> {
    await new Promise((r) => setTimeout(r, this.updateIntervalMs));

    if (this.endpoint) {
      const { name, maxPlayers, online } = this;
      await Axios.post(this.endpoint, { name, maxPlayers, online });
    }
  }

  connect(): void {
    ++this.online;
  }

  disconnect(): void {
    --this.online;
  }

  customPacket(): void {
    return;
  }

  private online = 0;
  private endpoint: string;
}
