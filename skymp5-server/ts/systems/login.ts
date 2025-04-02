import { System, Log, Content, SystemContext } from "./system";
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
    ctx: SystemContext
  ): void {
    if (type !== "loginWithSkympIo") return;

    const ip = ctx.svr.getUserIp(userId);
    console.log(`Connecting a user ${userId} with ip ${ip}`);

    const gameData = content["gameData"];
    if (this.offlineMode === true && gameData && gameData.session) {
      this.log("The server is in offline mode, the client is NOT");
    } else if (this.offlineMode === false && gameData && gameData.session) {
      const promise = this.performLoginOnlineMode(ctx, userId, gameData, ip);
      promise.catch((err) => {
        console.error("Error logging in client:", JSON.stringify(gameData), err)
      });
    } else if (this.offlineMode === true && gameData && typeof gameData.profileId === "number") {
      const profileId = gameData.profileId;
      this.emitEvent(ctx, "spawnAllowed", userId, profileId, [], undefined);
      this.log(userId + " logged as " + profileId);
    } else {
      this.log("No credentials found in gameData:", gameData);
    }
  }

  private async checkDiscordRoles(ctx: SystemContext, userId: number, profile: UserProfile): Promise<{ roles: string[], isIpHidden: boolean } | null> {
    const { discordAuth } = this.settingsObject;

    if (discordAuth && !discordAuth.botToken) {
      console.error("discordAuth.botToken is missing, skipping Discord server integration");
      return null;
    }
    if (discordAuth && !discordAuth.guildId) {
      console.error("discordAuth.guildId is missing, skipping Discord server integration");
      return null;
    }

    if (!discordAuth) {
      console.log("discordAuth is missing, skipping Discord server integration");
      return null;
    }

    // TODO: remove this check when Discord becomes optional
    if (!profile.discordId) {
      ctx.svr.sendCustomPacket(userId, loginFailedNotLoggedViaDiscord);
      throw new Error("Not logged in via Discord");
    }

    // if (!profile.discordId) {
    //   return null;
    // }

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
    const actorId = ctx.svr.getActorsByProfileId(profile.id)[0]; // TODO: what if more characters

    const receivedRoles: string[] | null = (responseData && Array.isArray(responseData.roles)) ? responseData.roles : null;
    const currentRoles: string[] | null = actorId ? mp.get(actorId, "private.discordRoles") : null;
    const roles = receivedRoles || currentRoles || [];

    console.log('Discord request:', JSON.stringify({ status: response.status, data: responseData }));

    // TODO: remove this check when Discord becomes optional
    if (response.status === 404 && responseData?.code === DiscordErrors.unknownMember) {
      ctx.svr.sendCustomPacket(userId, loginFailedNotInTheDiscordServer);
      throw new Error("Not in the Discord server");
    }

    // TODO: enable logging instead of throw
    // Disabled this check to be able bypassing ratelimit
    // if (response.status !== 200) {
    //   throw new Error("Unexpected response status: " +
    //     JSON.stringify({ status: response.status, data: response.data }));
    // }

    // TODO: remove this legacy discord-based ban system
    if (roles.indexOf(discordAuth.banRoleId) !== -1) {
      ctx.svr.sendCustomPacket(userId, loginFailedBanned);
      throw new Error("Banned");
    }

    return { roles, isIpHidden: roles.indexOf(discordAuth.hideIpRoleId) !== -1 };
  }

  private postLogsToDiscord(ctx: SystemContext, userId: number, profile: UserProfile, isIpHidden: boolean): Promise<void> {
    const { discordAuth } = this.settingsObject;

    if (!discordAuth.eventLogChannelId) {
      return;
    }

    const ipToPrint = isIpHidden ? "hidden" : ctx.svr.getUserIp(userId);

    const actorIds = ctx.svr.getActorsByProfileId(profile.id).map(actorId => actorId.toString(16));

    const loginMessage = `Server Login: Server Slot ${userId}, IP ${ipToPrint}, Actor ID ${actorIds}, Master API ${profile.id}, Discord ID ${profile.discordId} <@${profile.discordId}>`;
    console.log(loginMessage);

    this.fetchRetry(`https://discord.com/api/channels/${discordAuth.eventLogChannelId}/messages`, {
      method: 'POST',
      headers: {
        'Authorization': `${discordAuth.botToken}`,
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        content: loginMessage,
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

  private async performLoginOnlineMode(ctx: SystemContext, userId: number, gameData: any, initialIp: string): Promise<void> {
    this.emitEvent(ctx, "userAssignSession", userId, gameData.session);

    const guidBeforeAsyncOp = ctx.svr.getUserGuid(userId);
    const profile = await this.getUserProfile(gameData.session, userId, ctx);
    const guidAfterAsyncOp = ctx.svr.isConnected(userId) ? ctx.svr.getUserGuid(userId) : "<disconnected>";

    console.log({ guidBeforeAsyncOp, guidAfterAsyncOp, op: "getUserProfile" });

    if (guidBeforeAsyncOp !== guidAfterAsyncOp) {
      console.error(`User ${userId} changed guid from ${guidBeforeAsyncOp} to ${guidAfterAsyncOp} during async getUserProfile`);
      throw new Error("Guid mismatch after getUserProfile");
    }

    console.log("getUserProfileId:", profile);

    const discordRolesCheckResult = await this.checkDiscordRoles(ctx, userId, profile);

    if (initialIp !== ctx.svr.getUserIp(userId)) {
      // It's a quick and dirty way to check if it's the same user
      // During async http call the user could free userId and someone else could connect with the same userId
      ctx.svr.sendCustomPacket(userId, loginFailedIpMismatch);
      throw new Error("IP mismatch");
    }

    this.postLogsToDiscord(ctx, userId, profile, discordRolesCheckResult?.isIpHidden || false);

    if ((ctx.svr as any).onLoginAttempt) {
      const isContinue = (ctx.svr as any).onLoginAttempt(profile.id);
      if (!isContinue) {
        ctx.svr.sendCustomPacket(userId, loginFailedBanned);
        throw new Error("Banned by gamemode");
      }
    }

    this.emitEvent(ctx, "spawnAllowed", userId, profile.id, discordRolesCheckResult?.roles || [], profile.discordId);
    this.log("Logged as " + profile.id);
  }

  private emitEvent(ctx: SystemContext, eventName: string, ...args: unknown[]) {
    const emit = (ctx.gm as any).emit;
    emit(eventName, ...args);
  }

  private settingsObject: Settings;
  private fetchRetry = fetchRetry.default(global.fetch);
}
