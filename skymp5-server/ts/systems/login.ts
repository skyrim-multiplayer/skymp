import { System, Log, Content, SystemContext } from "./system";
import Axios from "axios";

const getMyPublicIp = async (): Promise<string> => {
  const res = await Axios.request({
    url: "http://ipv4bot.whatismyipaddress.com",
  });
  return res.data;
};

export class Login implements System {
  systemName = "Login";

  constructor(
    private log: Log,
    private maxPlayers: number,
    private masterUrl: string | null,
    private serverPort: number,
    private ip: string
  ) {}

  private async getUserProfileId(session: string): Promise<any> {
    return await Axios.get(
      `${this.masterUrl}/api/servers/${this.myAddr}/sessions/${session}`
    );
  }

  async initAsync(ctx: SystemContext): Promise<void> {
    this.userProfileIds.length = this.maxPlayers;
    this.userProfileIds.fill(undefined);

    if (this.ip && this.ip != "null")
      this.myAddr = this.ip + ":" + this.serverPort;
    else this.myAddr = (await getMyPublicIp()) + ":" + this.serverPort;
    this.log(
      `Login system assumed that ${this.myAddr} is our address on master`
    );

    ctx.gm.on("loginRequired", (userId: number) => {
      ctx.svr.sendCustomPacket(userId, "loginRequired", {});
      console.log("Login required for " + userId);
    });
  }

  disconnect(userId: number): void {
    this.userProfileIds[userId] = undefined;
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext
  ): void {
      if (type !== "loginWithSkympIo")
          return;
      this.log('userId=' + userId + ', our id will be' + 20 + userId);
      ctx.gm.emit("spawnAllowed", userId, 20 + userId);
  }

  private userProfileIds = new Array<undefined | number>();
  private myAddr: string;
}
