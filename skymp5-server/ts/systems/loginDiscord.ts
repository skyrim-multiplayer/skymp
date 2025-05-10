import { Settings } from "../settings";
import { Content, System, SystemContext } from "./system";

const fs = require('fs');
const path = require('path');
const express = require('express');


export class LoginDiscord implements System {
  systemName = "Login";

  private settingsObject: Settings;
  private app: any;
  async initAsync(ctx: SystemContext): Promise<void> {
    this.settingsObject = await Settings.get();

    this.app = express({ port: 3001 });
    const discordAuth = this.settingsObject.discordAuth;

    this.app.get('/api/auth/callback/discord', async ({ query }, response) => {
      const { code } = query;

      if (code) {
        try {
          const tokenResponseData = await fetch('https://discord.com/api/oauth2/token', {
            method: 'POST',
            body: new URLSearchParams({
              client_id: discordAuth.clientId,
              client_secret: discordAuth.botToken,
              code,
              grant_type: 'authorization_code',
              redirect_uri: discordAuth.callbackUrl,
              scope: 'identify',
            }).toString(),
            headers: {
              'Content-Type': 'application/x-www-form-urlencoded',
            },
          });

          const oauthData = await tokenResponseData.json();

          return response.redirect(`skyrim-rp://auth?access_token=${oauthData.access_token}`);
        } catch (error) {
          console.error(error);
          return response.send("OK");
        }
      }

      return response.send("OK");
    });

    this.app.listen(3001, () => console.log(`[+] Login Discord OAuth listening at http://localhost:${3001}`));
  }

  customPacket(
    userId: number,
    type: string,
    content: Content,
    ctx: SystemContext
  ): void {
    const ip = ctx.svr.getUserIp(userId);
    console.log(`Connecting a user ${userId} with ip ${ip}`);

    const gameData = content["gameData"];
    const accessToken = gameData?.accessToken;

    const profilesFilePath = path.join(__dirname, 'profiles.json');

    if (!fs.existsSync(profilesFilePath)) {
      fs.writeFileSync(profilesFilePath, JSON.stringify({
        lastIndex: 0,
        users: {}
      }));
    }

    if (!accessToken) {
      console.error('No access token provided');
      return;
    }

    fetch('https://discord.com/api/users/@me', {
      headers: {
        Authorization: `Bearer ${accessToken}`
      }
    })
      .then(response => {
        if (!response.ok) {
          throw new Error(`Discord API responded with status: ${response.status}`);
        }
        return response.json();
      })
      .then(userData => {
        const discordUserId = userData.id;

        let profilesData = JSON.parse(fs.readFileSync(profilesFilePath, 'utf8'));

        if (!profilesData.users) {
          profilesData = {
            lastIndex: 0,
            users: {}
          };
        }

        if (profilesData.users[discordUserId] !== undefined) {
          const profileId = profilesData.users[discordUserId];
          console.log(`Verified user ${discordUserId}. Using stored profileId: ${profileId}`);

          this.emit(ctx, "spawnAllowed", userId, profileId, [], undefined);
        } else {
          profilesData.lastIndex += 1;
          const profileId = profilesData.lastIndex;

          profilesData.users[discordUserId] = profileId;
          fs.writeFileSync(profilesFilePath, JSON.stringify(profilesData, null, 2));
          console.log(`Verified new Discord user ${discordUserId}. Created profileId: ${profileId}`);

          this.emit(ctx, "spawnAllowed", userId, profileId, [], undefined);
        }
      })
      .catch(error => {
        console.error('Error verifying Discord token:', error);
        // Optional: You could reject the connection here instead of using a temp profileId
        const tempProfileId = -1; // Using -1 for temporary profiles
        console.log(`Failed to verify user. Using temporary profileId: ${tempProfileId}`);
        this.emit(ctx, "spawnAllowed", userId, tempProfileId, [], undefined);
      });
  }

  private emit(ctx: SystemContext, eventName: string, ...args: unknown[]) {
    (ctx.gm as any).emit(eventName, ...args);
  }
}
