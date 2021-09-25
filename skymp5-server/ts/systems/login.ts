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
    if (type !== "loginWithSkympIo") return;

    const gameData = content["gameData"];
    if (gameData && gameData.session) {
      this.getUserProfileId(gameData.session).then((res) => {
        console.log("getUserProfileId", res.data);
        if (!res.data || !res.data.user || res.data.user.id === undefined)
          this.log("Bad master answer");
        else {
          this.userProfileIds[userId] = res.data.user.id;
          ctx.gm.emit("spawnAllowed", userId, res.data.user.id);
          this.log("Logged as " + res.data.user.id);
        }
      });
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private userProfileIds = new Array<undefined | number>();
  private myAddr: string;
}
