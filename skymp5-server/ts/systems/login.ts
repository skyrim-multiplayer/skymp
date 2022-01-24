import { System, Log, Content, SystemContext } from "./system";
import Axios from "axios";
import { getMyPublicIp } from "../publicIp";

export class Login implements System {
  systemName = "Login";

  constructor(
    private log: Log,
    private maxPlayers: number,
    private masterUrl: string | null,
    private serverPort: number,
    private ip: string,
    private offlineMode: boolean
  ) { }

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
    if (this.offlineMode === true && gameData && gameData.session) {
      this.log("The server is in offline mode, the client is NOT");
    }
    else if (this.offlineMode === false && gameData && gameData.session) {
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
    } else if (this.offlineMode === true && gameData && typeof gameData.profileId === "number") {
      const profileId = gameData.profileId;
      ctx.gm.emit("spawnAllowed", userId, profileId);
      this.log(userId + " logged as " + profileId);
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private userProfileIds = new Array<undefined | number>();
  private myAddr: string;
}
