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
    private serverPort: number
  ) {}

  private async getUserProfileId(session: string): Promise<any> {
    let res;
    for (const addr of [`${this.myIp}:${this.serverPort}`, "127.0.0.1:7777"])
      try {
        res = await Axios.get(
          `${this.masterUrl}/api/servers/${addr}/sessions/${session}`
        );
        break;
      } catch (e) {}

    return res;
  }

  async initAsync(ctx: SystemContext): Promise<void> {
    this.userProfileIds.length = this.maxPlayers;
    this.userProfileIds.fill(undefined);

    this.myIp = await getMyPublicIp();

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
        if (!res.data.user || res.data.user.id === undefined)
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
  private myIp: string;
}
