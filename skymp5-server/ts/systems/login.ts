import { System, Log, Content, SystemContext } from "./system";
import Axios from "axios";
import { getMyPublicIp } from "../publicIp";
import { Settings } from "../settings";

interface UserProfile {
  id: number;
  discordId: string | null;
}

namespace DiscordErrors {
  export const unknownMember = 10007;
}

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

  private async getUserProfile(session: string): Promise<UserProfile> {
    const response = await Axios.get(
      `${this.masterUrl}/api/servers/${this.myAddr}/sessions/${session}`
    );
    if (!response.data || !response.data.user || !response.data.user.id) {
      throw new Error("getUserProfile: bad master-api response");
    }
    return response.data.user as UserProfile;
  }

  async initAsync(ctx: SystemContext): Promise<void> {
    if (this.ip && this.ip != "null") {
      this.myAddr = this.ip + ":" + this.serverPort;
    } else {
      this.myAddr = (await getMyPublicIp()) + ":" + this.serverPort;
    }
    this.log(
      `Login system assumed that ${this.myAddr} is our address on master`
    );
  }

  disconnect(userId: number): void {
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext
  ): void {
    if (type !== "loginWithSkympIo") return;

    const ip = ctx.svr.getUserIp(userId);
    console.log(`Connecting a user ${userId} with ip ${ip}`);

    let discordAuth = Settings.get().discordAuth;

    const gameData = content["gameData"];
    if (this.offlineMode === true && gameData && gameData.session) {
      this.log("The server is in offline mode, the client is NOT");
    } else if (this.offlineMode === false && gameData && gameData.session) {
      (async () => {
        const profile = await this.getUserProfile(gameData.session);
        console.log("getUserProfileId:", profile);

        let roles = new Array<string>();

        if (discordAuth && !discordAuth.botToken) {
          discordAuth = undefined;
          console.error("discordAuth.botToken is missing, skipping Discord server integration");
        }
        if (discordAuth && !discordAuth.guildId) {
          discordAuth = undefined;
          console.error("discordAuth.guildId is missing, skipping Discord server integration");
        }

        if (discordAuth) {
          if (!profile.discordId) {
            throw new Error("Not logged in via Discord");
          }
          const response = await Axios.get(
            `https://discord.com/api/guilds/${discordAuth.guildId}/members/${profile.discordId}`,
            {
              headers: {
                'Authorization': `${discordAuth.botToken}`,
              },
              validateStatus: (status) => true,
            },
          );
          console.log('Discord request:', JSON.stringify({ status: response.status, data: response.data }));

          if (discordAuth.eventLogChannelId) {
            let ipToPrint = ip;

            if (discordAuth && discordAuth.hideIpRoleId) {
              if (response.data.roles.indexOf(discordAuth.hideIpRoleId) !== -1) {
                ipToPrint = "hidden";
              }
            }

            const actorIds = ctx.svr.getActorsByProfileId(profile.id).map(actorId => actorId.toString(16));
            Axios.post(
              `https://discord.com/api/channels/${discordAuth.eventLogChannelId}/messages`,
              {
                content: `Server Login: IP ${ipToPrint}, Actor ID ${actorIds}, Master API ${profile.id}, Discord ID ${profile.discordId} <@${profile.discordId}>`,
                allowed_mentions: { parse: [] },
              },
              {
                headers: {
                  'Authorization': `${discordAuth.botToken}`,
                },
              },
            ).catch((err) => console.error("Error sending message to Discord:", err));
          }

          if (response.status === 404 && response.data?.code === DiscordErrors.unknownMember) {
            throw new Error("Not on the Discord server");
          }
          if (response.status !== 200 || !response.data?.roles) {
            throw new Error("Unexpected response status: " +
                JSON.stringify({ status: response.status, data: response.data }));
          }
          if (response.data.roles.indexOf(discordAuth.banRoleId) !== -1) {
            throw new Error("Banned");
          }
          if (ip !== ctx.svr.getUserIp(userId)) {
            throw new Error("IP mismatch");
          }
          roles = response.data.roles;
        }
        ctx.gm.emit("spawnAllowed", userId, profile.id, roles, profile.discordId);
        this.log("Logged as " + profile.id);
      })()
        .catch((err) => console.error("Error logging in client:", JSON.stringify(gameData), err));
    } else if (this.offlineMode === true && gameData && typeof gameData.profileId === "number") {
      const profileId = gameData.profileId;
      ctx.gm.emit("spawnAllowed", userId, profileId, []);
      this.log(userId + " logged as " + profileId);
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private myAddr: string;
}
