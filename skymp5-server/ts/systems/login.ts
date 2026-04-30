import { System, Log, Content, SystemContext } from "./system";
import { Settings } from "../settings";
import * as fetchRetry from "fetch-retry";
import { loginsCounter, loginErrorsCounter } from "./metricsSystem";

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

// See also NetworkingCombined.h
// In NetworkingCombined.h, we implement a hack to prevent the soul-transmission bug
// TODO: reimplement Login system. Preferably, in C++ with clear data flow.
export class Login implements System {
  systemName = "Login";

  constructor(
    private log: Log,
    private maxPlayers: number,
    private masterUrl: string | null,
    private serverPort: number,
    private masterKey: string,
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
      `${this.masterUrl}/api/servers/${this.masterKey}/sessions/${session}`,
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

    this.log(
      `Login system assumed that ${this.masterKey} is our master api key`
    );
  }

  disconnect(userId: number): void {
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext,
  ): void {
    if (type !== "loginWithSkympIo") {
      return;
    }

    const ip = ctx.svr.getUserIp(userId);
    console.log(`Connecting a user ${userId} with ip ${ip}`);

    let discordAuth = this.settingsObject.discordAuth;

    const gameData = content["gameData"];
    if (this.offlineMode === true && gameData && gameData.session) {
      this.log("The server is in offline mode, the client is NOT");
    } else if (this.offlineMode === false && gameData && gameData.session) {
      (async () => {
        this.emit(ctx, "userAssignSession", userId, gameData.session);

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

        if (discordAuth && (!discordAuth.guilds || discordAuth.guilds.length === 0)) {
          discordAuth = undefined;
          console.error("discordAuth.guilds array is missing or empty, skipping Discord server integration");
        }

        if ((ctx.svr as any).onLoginAttempt) {
          const isContinue = (ctx.svr as any).onLoginAttempt(profile.id);
          if (!isContinue) {
            ctx.svr.sendCustomPacket(userId, loginFailedBanned);
            throw new Error("Banned by gamemode");
          }
        }

        let roles = new Array<string>();

        if (discordAuth && discordAuth.botToken && discordAuth.guilds) {
          let isMemberOfAny = false;
          let roles = new Array<string>();
          let isBanned = false;
          let shouldHideIp = false;

          const actorId = ctx.svr.getActorsByProfileId(profile.id)[0];
          const mp = ctx.svr as unknown as Mp;
          const currentRoles: string[] | null = actorId ? mp.get(actorId, "private.discordRoles") : null;

          if (currentRoles && currentRoles.length > 0) {
            roles = currentRoles;
          }

          for (const guildConfig of discordAuth.guilds) {
            const response = await this.fetchRetry(
              `https://discord.com/api/guilds/${guildConfig.guildId}/members/${profile.discordId}`,
              {
                method: 'GET',
                headers: { 'Authorization': `${discordAuth.botToken}` },
                ...this.getFetchOptions('discordAuth_multi'),
              },
            );

            if (response.ok) {
              const responseData = await response.json();
              isMemberOfAny = true;

              const guildRoles: string[] = responseData.roles || [];
              roles = [...new Set([...roles, ...guildRoles])];
              if (guildConfig.banRoleId && guildRoles.indexOf(guildConfig.banRoleId) !== -1) {
                isBanned = true;
              }
              if (guildConfig.hideIpRoleId && guildRoles.indexOf(guildConfig.hideIpRoleId) !== -1) {
                shouldHideIp = true;
              }
            }

            // TODO: enable logging instead of throw
            // Disabled this check to be able bypassing ratelimit
            // if (response.status !== 200) {
            //   throw new Error("Unexpected response status: " +
            //     JSON.stringify({ status: response.status, data: response.data }));
            // }
          }


          if (!isMemberOfAny) {
            ctx.svr.sendCustomPacket(userId, loginFailedNotInTheDiscordServer);
            throw new Error("Not in any of the Discord servers");
          }

          if (isBanned) {
            ctx.svr.sendCustomPacket(userId, loginFailedBanned);
            throw new Error("Banned on one of the Discord servers");
          }


          if (discordAuth && profile.discordId) {
            if (ip !== ctx.svr.getUserIp(userId)) {
              // It's a quick and dirty way to check if it's the same user
              // During async http call the user could free userId and someone else could connect with the same userId
              ctx.svr.sendCustomPacket(userId, loginFailedIpMismatch);
              throw new Error("IP mismatch");
            }
          }

          const ipToPrint = shouldHideIp ? "hidden" : ip;
          const actorIds = ctx.svr.getActorsByProfileId(profile.id).map(id => id.toString(16));

          for (const guildConfig of discordAuth.guilds) {
            if (guildConfig.eventLogChannelId) {
              this.postServerLoginToDiscord(guildConfig.eventLogChannelId, discordAuth.botToken, {
                userId,
                ipToPrint,
                actorIds,
                profile,
              });
            }
          }
        }

        this.emit(ctx, "spawnAllowed", userId, profile.id, roles, profile.discordId);
        loginsCounter.inc();
        this.log("Logged as " + profile.id);
      })()
        .catch((err) => {
          loginErrorsCounter.inc({ reason: err?.message || "unknown" });
          console.error("Error logging in client:", JSON.stringify(gameData), err)
        });
    } else if (this.offlineMode === true && gameData && typeof gameData.profileId === "number") {
      const profileId = gameData.profileId;
      this.emit(ctx, "spawnAllowed", userId, profileId, [], undefined);
      loginsCounter.inc();
      this.log(userId + " logged as " + profileId);
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private postServerLoginToDiscord(eventLogChannelId: string, botToken: string, options: { userId: number, ipToPrint: string, actorIds: string[], profile: UserProfile }) {
    const { userId, ipToPrint, actorIds, profile } = options;

    const loginMessage = `Server Login: Server Slot ${userId}, IP ${ipToPrint}, Actor ID ${actorIds}, Master API ${profile.id}, Discord ID ${profile.discordId} <@${profile.discordId}>`;
    console.log(loginMessage);

    this.fetchRetry(`https://discord.com/api/channels/${eventLogChannelId}/messages`, {
      method: 'POST',
      headers: {
        'Authorization': `${botToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        content: loginMessage,
        allowed_mentions: { parse: [] },
      }),
      ... this.getFetchOptions('discordAuth2'),
    }).then((response) => {
      if (!response.ok) {
        throw new Error(`Error sending message to Discord: ${response.statusText}`);
      }
      return response.json();
    }).then((_data): null => {
      return null;
    }).catch((err) => {
      console.error("Error sending message to Discord:", err);
    });
  }

  private emit(ctx: SystemContext, eventName: string, ...args: unknown[]) {
    (ctx.gm as any).emit(eventName, ...args);
  }

  private settingsObject: Settings;
  private fetchRetry = fetchRetry.default(global.fetch);
}
