import { System, Log, Content, SystemContext } from "./system";
import { getMyPublicIp } from "../publicIp";
import { Settings } from "../settings";
import * as fetchRetry from "fetch-retry";

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

  private getFetchOptions(callerFunctionName: string) {
    return {
      // retry on any network error, or 5xx status codes
      retryOn: (attempt: number, error: Error | null, response: Response) => {
        const retry = error !== null || response.status >= 500;
        if (retry) {
          console.log(`${callerFunctionName}: retrying request ${JSON.stringify({ attempt, error, status: response.status })}`);
        }
        return retry;
      },
      retries: 10
    };
  }

  private async getUserProfile(session: string, userId: number, ctx: SystemContext): Promise<UserProfile> {
    const response = await this.fetchRetry(
      `${this.masterUrl}/api/servers/${this.myAddr}/sessions/${session}`,
      this.getFetchOptions('getUserProfile')
    );

    if (!response.ok) {
      if (response.status === 404) {
        ctx.svr.sendCustomPacket(userId, loginFailedSessionNotFound);
      }
      throw new Error(`getUserProfile: HTTP error ${response.status}`);
    }

    const data = await response.json();

    if (!data || !data.user || !data.user.id) {
      throw new Error(`getUserProfile: bad master-api response ${JSON.stringify(data)}`);
    }

    return data.user as UserProfile;
  }

  async initAsync(ctx: SystemContext): Promise<void> {
    this.settingsObject = await Settings.get();

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

    let discordAuth = this.settingsObject.discordAuth;

    const gameData = content["gameData"];
    if (this.offlineMode === true && gameData && gameData.session) {
      this.log("The server is in offline mode, the client is NOT");
    } else if (this.offlineMode === false && gameData && gameData.session) {
      (async () => {
        ctx.gm.emit("userAssignSession", userId, gameData.session);

        const guidBeforeAsyncOp = ctx.svr.getUserGuid(userId);
        const profile = await this.getUserProfile(gameData.session, userId, ctx);
        const guidAfterAsyncOp = ctx.svr.isConnected(userId) ? ctx.svr.getUserGuid(userId) : "<disconnected>";

        console.log({ guidBeforeAsyncOp, guidAfterAsyncOp, op: "getUserProfile" });

        if (guidBeforeAsyncOp !== guidAfterAsyncOp) {
          console.error(`User ${userId} changed guid from ${guidBeforeAsyncOp} to ${guidAfterAsyncOp} during async getUserProfile`);
          throw new Error("Guid mismatch after getUserProfile");
        }

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
          const guidBeforeAsyncOp = ctx.svr.getUserGuid(userId);
          const response = await this.fetchRetry(
            `https://discord.com/api/guilds/${discordAuth.guildId}/members/${profile.discordId}`,
            {
              method: 'GET',
              headers: {
                'Authorization': `${discordAuth.botToken}`,
              },
              ... this.getFetchOptions('discordAuth1'),
            },
          );
          const responseData = response.ok ? await response.json() : null;
          const guidAfterAsyncOp = ctx.svr.isConnected(userId) ? ctx.svr.getUserGuid(userId) : "<disconnected>";

          console.log({ guidBeforeAsyncOp, guidAfterAsyncOp, op: "Discord request" });

          if (guidBeforeAsyncOp !== guidAfterAsyncOp) {
            console.error(`User ${userId} changed guid from ${guidBeforeAsyncOp} to ${guidAfterAsyncOp} during async Discord request`);
            throw new Error("Guid mismatch after Discord request");
          }

          const mp = ctx.svr as unknown as Mp;

          // TODO: what if more characters
          const actorId = ctx.svr.getActorsByProfileId(profile.id)[0];

          const receivedRoles: string[] | null = (responseData && Array.isArray(responseData.roles)) ? responseData.roles : null;
          const currentRoles: string[] | null = actorId ? mp.get(actorId, "private.discordRoles") : null;
          roles = receivedRoles || currentRoles || [];

          console.log('Discord request:', JSON.stringify({ status: response.status, data: responseData }));

          if (discordAuth.eventLogChannelId) {
            let ipToPrint = ip;

            if (discordAuth && discordAuth.hideIpRoleId) {
              if (roles.indexOf(discordAuth.hideIpRoleId) !== -1) {
                ipToPrint = "hidden";
              }
            }

            const actorIds = ctx.svr.getActorsByProfileId(profile.id).map(actorId => actorId.toString(16));
            this.fetchRetry(`https://discord.com/api/channels/${discordAuth.eventLogChannelId}/messages`, {
              method: 'POST',
              headers: {
                'Authorization': `${discordAuth.botToken}`,
                'Content-Type': 'application/json',
              },
              body: JSON.stringify({
                content: `Server Login: IP ${ipToPrint}, Actor ID ${actorIds}, Master API ${profile.id}, Discord ID ${profile.discordId} <@${profile.discordId}>`,
                allowed_mentions: { parse: [] },
              }),
              ... this.getFetchOptions('discordAuth2'),
            })
              .then((response) => {
                if (!response.ok) {
                  throw new Error(`Error sending message to Discord: ${response.statusText}`);
                }
                return response.json();
              })
              .then((_data) => null)
              .catch((err) => {
                console.error("Error sending message to Discord:", err);
              });
          }

          if (response.status === 404 && responseData?.code === DiscordErrors.unknownMember) {
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
  private settingsObject: Settings;
  private fetchRetry = fetchRetry.default(global.fetch);
}
