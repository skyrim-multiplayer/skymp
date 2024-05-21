import { System, Log, Content, SystemContext } from "./system";
import Axios from "axios";
import { getMyPublicIp } from "../publicIp";
import { Settings } from "../settings";

const loginFailedNotLoggedViaDiscord = JSON.stringify({ customPacketType: "loginFailedNotLoggedViaDiscord" });
const loginFailedNotInTheDiscordServer = JSON.stringify({ customPacketType: "loginFailedNotInTheDiscordServer" });
const loginFailedBanned = JSON.stringify({ customPacketType: "loginFailedBanned" });
const loginFailedIpMismatch = JSON.stringify({ customPacketType: "loginFailedIpMismatch" });
const loginFailedSessionNotFound = JSON.stringify({ customPacketType: "loginFailedSessionNotFound" });

type Mp = any; // TODO

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

  private async getUserProfile(session: string, userId: number, ctx: SystemContext): Promise<UserProfile> {
    try {
      const response = await Axios.get(
        `${this.masterUrl}/api/servers/${this.myAddr}/sessions/${session}`
      );
      if (!response.data || !response.data.user || !response.data.user.id) {
        throw new Error("getUserProfile: bad master-api response");
      }
      return response.data.user as UserProfile;
    } catch (error) {
      if (Axios.isAxiosError(error) && error.response?.status === 404) {
        ctx.svr.sendCustomPacket(userId, loginFailedSessionNotFound);
      }
      throw error;
    }
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
        const profile = await this.getUserProfile(gameData.session, userId, ctx);
        console.log("getUserProfileId:", profile);

        if (discordAuth && !discordAuth.botToken) {
          discordAuth = undefined;
          console.error("discordAuth.botToken is missing, skipping Discord server integration");
        }
        if (discordAuth && !discordAuth.guildId) {
          discordAuth = undefined;
          console.error("discordAuth.guildId is missing, skipping Discord server integration");
        }

        let roles = new Array<string>();

        if (discordAuth) {
          if (!profile.discordId) {
            ctx.svr.sendCustomPacket(userId, loginFailedNotLoggedViaDiscord);
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

          const mp = ctx.svr as unknown as Mp;

          // TODO: what if more characters
          const actorId = ctx.svr.getActorsByProfileId(profile.id)[0];

          const receivedRoles: string[] | null = (response.data && Array.isArray(response.data.roles)) ? response.data.roles : null;
          const currentRoles: string[] | null = actorId ? mp.get(actorId, "private.discordRoles") : null;
          roles = receivedRoles || currentRoles || [];

          console.log('Discord request:', JSON.stringify({ status: response.status, data: response.data }));

          if (discordAuth.eventLogChannelId) {
            let ipToPrint = ip;

            if (discordAuth && discordAuth.hideIpRoleId) {
              if (roles.indexOf(discordAuth.hideIpRoleId) !== -1) {
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
            ctx.svr.sendCustomPacket(userId, loginFailedNotInTheDiscordServer);
            throw new Error("Not in the Discord server");
          }
          // Disabled this check to be able bypassing ratelimit
          // if (response.status !== 200) {
          //   throw new Error("Unexpected response status: " +
          //     JSON.stringify({ status: response.status, data: response.data }));
          // }
          if (roles.indexOf(discordAuth.banRoleId) !== -1) {
            ctx.svr.sendCustomPacket(userId, loginFailedBanned);
            throw new Error("Banned");
          }
          if (ip !== ctx.svr.getUserIp(userId)) {
            // It's a quick and dirty way to check if it's the same user
            // During async http call the user could free userId and someone else could connect with the same userId
            ctx.svr.sendCustomPacket(userId, loginFailedIpMismatch);
            throw new Error("IP mismatch");
          }
        }
        ctx.gm.emit("spawnAllowed", userId, profile.id, roles, profile.discordId);
        this.log("Logged as " + profile.id);
      })()
        .catch((err) => {
          console.error("Error logging in client:", JSON.stringify(gameData), err)
        });
    } else if (this.offlineMode === true && gameData && typeof gameData.profileId === "number") {
      const profileId = gameData.profileId;
      ctx.gm.emit("spawnAllowed", userId, profileId, [], undefined);
      this.log(userId + " logged as " + profileId);
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private myAddr: string;
}
